/*
 * Copyright (c) 2025 arvindkrv@protonmail.com
 *
 * Please see the LICENSE file for the terms and conditions
 * associated with this software.
 */

#include <cstdint>
#include <iostream>
#include <map>
#include <optional>
#include <unordered_map>

#include "../../utils/logger/log_level.h"
#include "../../utils/logger/logger.h"
#include "../../utils/math/math_utils.h"
#include "../market_data/market_data_feed.h"
#include "../trading/asset_config.h"
#include "../trading/depth.h"
#include "../types/enums/action_type.h"
#include "../types/enums/order_status.h"
#include "backtest_asset.h"
#include "backtest_engine.h"

namespace core::backtest {
/**
 * @brief Constructs a BacktestEngine with per-asset book/trade streams and
 * configurations.
 *
 * This constructor initializes the market data feed and execution engine for
 * multiple assets using the provided CSV file paths and asset configuration
 * data. It also prepares internal state such as position tracking, trade
 * counters, and PnL for each asset.
 *
 * @param book_files A map from asset ID to CSV file path containing L2 book
 * updates.
 * @param trade_files A map from asset ID to CSV file path containing trade
 * events.
 * @param asset_configs A map from asset ID to AssetConfig objects defining tick
 * size, lot size, etc.
 *
 * @note All assets in @p asset_configs are expected to have corresponding
 * entries in @p book_files. Trade file entries are optional but recommended.
 */
BacktestEngine::BacktestEngine(
    const std::unordered_map<int, core::trading::AssetConfig> &asset_configs,
    const core::backtest::BacktestEngineConfig &engine_config,
    std::shared_ptr<utils::logger::Logger> logger)
    : current_time_us_(0), local_cash_balance_(engine_config.initial_cash_),
      logger_(logger) {
    using namespace core::market_data;
    using namespace core::backtest;
    order_entry_latency_us = engine_config.order_entry_latency_us_;
    order_response_latency_us = engine_config.order_response_latency_us_;
    market_feed_latency_us = engine_config.market_feed_latency_us_;

    execution_engine_ = core::execution_engine::ExecutionEngine(logger_);
    execution_engine_.set_order_entry_latency_us(order_entry_latency_us);
    execution_engine_.set_order_response_latency_us(order_response_latency_us);

    for (const auto &[asset_id, config] : asset_configs) {
        using namespace core::orderbook;

        assets_.emplace(asset_id, BacktestAsset(config));
        execution_engine_.add_asset(asset_id, config.tick_size_,
                                    config.lot_size_);
        market_data_feed_.add_stream(asset_id, config.book_update_file_,
                                     config.trade_file_);
        local_orderbooks_.emplace(
            asset_id, OrderBook(config.tick_size_, config.lot_size_, logger_));

        num_trades_[asset_id] = 0;
        trading_volume_[asset_id] = 0.0;
        trading_value_[asset_id] = 0.0;
        realized_pnl_[asset_id] = 0.0;
        local_position_[asset_id] = 0.0;

        tick_sizes_[asset_id] = config.tick_size_;
        lot_sizes_[asset_id] = config.lot_size_;
    }
    auto first_event_us_opt = market_data_feed_.peek_timestamp();
    if (first_event_us_opt.has_value()) {
        std::uint64_t raw_start =
            *first_event_us_opt > 1000000 ? *first_event_us_opt - 1000000 : 0;
        current_time_us_ = (raw_start / 1000000) * 1000000;
    } else {
        current_time_us_ = 0;
    }
    if (logger_) {
        logger_->log("[BacktestEngine] - Initialization: assets=" +
                         std::to_string(assets_.size()) +
                         ", order_entry_latency_us=" +
                         std::to_string(order_entry_latency_us) +
                         ", order_response_latency_us=" +
                         std::to_string(order_response_latency_us) +
                         ", market_feed_latency_us=" +
                         std::to_string(market_feed_latency_us),
                     utils::logger::LogLevel::Info);
    }
}

/**
 * @brief Advances the simulated clock and processes all events and delayed
 * actions.
 *
 * This function simulates the passage of time in the backtest engine by
 * advancing the internal clock by the given number of microseconds. It
 * processes:
 * - All delayed actions (e.g., order submissions, cancellations) scheduled to
 *   occur before the next market event or the end of the interval.
 * - The internal timestamp is updated accordingly.
 *
 * If the next event timestamp in the market data feed is beyond the interval,
 * only delayed actions within the current window are processed.
 *
 * @param microseconds The amount of simulated time to elapse (in microseconds).
 * @return true Always returns true (indicating the clock has moved forward).
 *
 * @throws std::invalid_argument If an unknown action type is encountered.
 */
bool BacktestEngine::elapse(std::uint64_t microseconds) {
    using namespace core::market_data;
    EventType event_type;
    BookUpdate book_update;
    Trade trade;
    int asset_id;
    auto next_interval_us = current_time_us_ + microseconds;
    while (current_time_us_ < next_interval_us) {
        auto next_event_us_opt = market_data_feed_.peek_timestamp();
        Timestamp next_event_us = std::numeric_limits<Timestamp>::max();
        if (next_event_us_opt.has_value()) next_event_us = *next_event_us_opt;
        // Get all delayed actions scheduled between now and next market event
        auto it = delayed_actions_.lower_bound(current_time_us_);
        Timestamp interval_end_us = std::min(next_event_us, next_interval_us);

        while (it != delayed_actions_.end() && it->first < interval_end_us) {
            const DelayedAction &action = it->second;
            current_time_us_ = action.execute_time_;
            switch (action.type_) {
            // exchange events
            case ActionType::SubmitBuy:
                execution_engine_.execute_order(action.asset_id_,
                                                TradeSide::Buy, *action.order_);
                break;
            case ActionType::SubmitSell:
                execution_engine_.execute_order(
                    action.asset_id_, TradeSide::Sell, *action.order_);
                break;
            case ActionType::Cancel:
                execution_engine_.cancel_order(
                    action.asset_id_, *action.orderId_, current_time_us_);
                break;
            // local events
            case ActionType::LocalProcessFill:
                process_fill_local(action.asset_id_, *action.fill_);
                break;
            case ActionType::LocalBookUpdate:
                process_book_update_local(action.asset_id_,
                                          *action.book_update_);
                break;
            case ActionType::LocalOrderUpdate:
                process_order_update_local(*action.order_update_type_,
                                           *action.orderId_, *action.order_);
                break;
            default:
                throw std::invalid_argument(
                    "Unknown ActionType in DelayedAction");
            }
            // process any fills or order updates in the exchange events
            process_exchange_fills();
            process_exchange_order_updates();
            ++it;
        }
        // process another event before interval ends
        if (next_event_us < next_interval_us) {
            market_data_feed_.next_event(asset_id, event_type, book_update,
                                         trade);
            if (event_type == EventType::Trade) {
                execution_engine_.handle_trade(asset_id, trade);
                process_exchange_fills();
                process_exchange_order_updates();
            } else if (event_type == EventType::BookUpdate) {
                execution_engine_.handle_book_update(asset_id, book_update);
                // update local books with feed latency
                delayed_actions_.insert(
                    {book_update.local_timestamp_,
                     DelayedAction{.type_ = ActionType::LocalBookUpdate,
                                   .asset_id_ = asset_id,
                                   .order_ = std::nullopt,
                                   .orderId_ = std::nullopt,
                                   .order_update_type_ = std::nullopt,
                                   .fill_ = std::nullopt,
                                   .book_update_ = book_update,
                                   .execute_time_ =
                                       book_update.local_timestamp_}});
            } else {
                std::invalid_argument("Incorrect EventType");
            }
            current_time_us_ = next_event_us;
        } else {
            current_time_us_ = next_interval_us;
        }
    }
    current_time_us_ = next_interval_us;
    if (logger_) {
        logger_->log("[BacktestEngine] - " + std::to_string(current_time_us_) +
                         "us - elapse complete",
                     utils::logger::LogLevel::Debug);
    }
    return std::isfinite(current_time_us_);
}

bool BacktestEngine::order_inactive(const core::trading::Order &order) {
    if (order.orderStatus_ == OrderStatus::FILLED ||
        order.orderStatus_ == OrderStatus::CANCELLED ||
        order.orderStatus_ == OrderStatus::EXPIRED) {
        return true;
    }
    return false;
}
/**
 * @brief Clears cancelled, filled, or expired orders.
 * @param asset_id
 */
void BacktestEngine::clear_inactive_orders() {
    using namespace core::trading;
    using namespace utils::logger;
    if (logger_) {
        logger_->log("[BacktestEngine] - " + std::to_string(current_time_us_) +
                         "us - clearing inactive orders",
                     LogLevel::Debug);
    }
    for (auto &asset_pair : assets_) {
        int asset_id = asset_pair.first;
        execution_engine_.clear_inactive_orders(asset_id);
    }
    for (auto it = local_active_orders_.begin();
         it != local_active_orders_.end();) {
        if (order_inactive(it->second)) {
            if (logger_) {
                logger_->log("[BacktestEngine] - " +
                                 std::to_string(current_time_us_) +
                                 "us - clearing inactive order (" +
                                 std::to_string(it->second.orderId_) + ")",
                             LogLevel::Debug);
            }
            it = local_active_orders_.erase(it);
        } else {
            ++it;
        }
    }
}

/**
 * @brief Submits a buy order to the backtest engine with simulated latency.
 *
 * A buy order is submitted by the local system. This method processes
 * the buy order and adds order entry latency before sending to the
 * execution engine (exchange) to process.
 *
 * @param asset_id The ID of the asset for which the order is submitted.
 * @param price The price at which the buy order is placed.
 * @param quantity The quantity of the asset to buy.
 * @param tif The time-in-force policy for the order (e.g., FOK, IOC, GTX).
 * @param orderType The type of the order (e.g., LIMIT, MARKET).
 * @return The unique order ID assigned to the submitted order.
 */
OrderId BacktestEngine::submit_buy_order(int asset_id, Price price,
                                         Quantity quantity, TimeInForce tif,
                                         OrderType orderType) {
    using namespace core::trading;
    if (quantity <= 0.0) throw std::invalid_argument("Insufficient quantity");
    if (orderType == OrderType::LIMIT && price <= 0.0)
        throw std::invalid_argument("Invalid price for limit order");
    Order buy_order{.local_timestamp_ = current_time_us_,
                    .exch_timestamp_ =
                        current_time_us_ + order_entry_latency_us,
                    .orderId_ = orderId_gen_.nextId(),
                    .side_ = BookSide::Bid,
                    .price_ = price,
                    .quantity_ = quantity,
                    .filled_quantity_ = 0.0,
                    .tif_ = tif,
                    .orderType_ = orderType,
                    .queueEst_ = 0.0,
                    .orderStatus_ = OrderStatus::NEW};
    if (logger_) {
        logger_->log("[BacktestEngine] - " + std::to_string(current_time_us_) +
                         "us - buy order (" +
                         std::to_string(buy_order.orderId_) +
                         ") submitted to exchange",
                     utils::logger::LogLevel::Debug);
    }
    delayed_actions_.insert(
        {buy_order.exch_timestamp_,
         DelayedAction{.type_ = ActionType::SubmitBuy,
                       .asset_id_ = asset_id,
                       .order_ = buy_order,
                       .orderId_ = std::nullopt,
                       .order_update_type_ = std::nullopt,
                       .fill_ = std::nullopt,
                       .book_update_ = std::nullopt,
                       .execute_time_ = buy_order.exch_timestamp_}});
    return buy_order.orderId_;
}

/**
 * @brief Submits a sell order to the backtest engine with simulated latency.
 *
 * Sell order is submitted from the local system. This method recieves the
 * order and adds order entry latency before letting the order be
 * processed by the execution engine (exchange).
 *
 * @param asset_id The ID of the asset for which the order is submitted.
 * @param price The price at which the sell order is placed.
 * @param quantity The quantity of the asset to sell.
 * @param tif The time-in-force policy for the order (e.g., FOK, IOC, GTX).
 * @param orderType The type of the order (e.g., LIMIT, MARKET).
 * @return The unique order ID assigned to the submitted order.
 */
OrderId BacktestEngine::submit_sell_order(int asset_id, Price price,
                                          Quantity quantity, TimeInForce tif,
                                          OrderType orderType) {
    using namespace core::trading;
    if (quantity <= 0.0) throw std::invalid_argument("Insufficient quantity");
    if (orderType == OrderType::LIMIT && price <= 0.0)
        throw std::invalid_argument("Invalid price for limit order");
    Order sell_order{.local_timestamp_ = current_time_us_,
                     .exch_timestamp_ =
                         current_time_us_ + order_entry_latency_us,
                     .orderId_ = orderId_gen_.nextId(),
                     .side_ = BookSide::Ask,
                     .price_ = price,
                     .quantity_ = quantity,
                     .filled_quantity_ = 0.0,
                     .tif_ = tif,
                     .orderType_ = orderType,
                     .queueEst_ = 0.0,
                     .orderStatus_ = OrderStatus::NEW};
    if (logger_) {
        logger_->log("[BacktestEngine] - " + std::to_string(current_time_us_) +
                         "us - sell order (" +
                         std::to_string(sell_order.orderId_) +
                         ") submitted to exchange",
                     utils::logger::LogLevel::Debug);
    }
    delayed_actions_.insert(
        {sell_order.exch_timestamp_,
         DelayedAction{.type_ = ActionType::SubmitSell,
                       .asset_id_ = asset_id,
                       .order_ = sell_order,
                       .orderId_ = std::nullopt,
                       .order_update_type_ = std::nullopt,
                       .fill_ = std::nullopt,
                       .book_update_ = std::nullopt,
                       .execute_time_ = sell_order.exch_timestamp_}});

    return sell_order.orderId_;
}

/**
 * @brief Submits a cancel request to the backtest engine with latency.
 *
 * Local originated, received by the exchange with order entry latency.
 */
void BacktestEngine::cancel_order(int asset_id, OrderId orderId) {
    delayed_actions_.insert(
        {current_time_us_ + order_response_latency_us,
         DelayedAction{.type_ = ActionType::Cancel,
                       .asset_id_ = asset_id,
                       .order_ = std::nullopt,
                       .orderId_ = orderId,
                       .order_update_type_ = std::nullopt,
                       .fill_ = std::nullopt,
                       .book_update_ = std::nullopt,
                       .execute_time_ =
                           current_time_us_ + order_entry_latency_us}});
}

/**
 * @brief Order response latency for order updates to local.
 *
 * Order changes are executed on the exchange (execution engine),
 * they are then reflected in the local (backtest engine) orders
 * after order response latency.
 */
void BacktestEngine::process_exchange_order_updates() {
    using namespace core::trading;
    using namespace core::execution_engine;
    std::vector<OrderUpdate> order_updates = execution_engine_.order_updates();
    for (const auto &order_update : order_updates) {
        delayed_actions_.insert(
            {order_update.local_timestamp_,
             DelayedAction{.type_ = ActionType::LocalOrderUpdate,
                           .asset_id_ = order_update.asset_id_,
                           .order_ = *order_update.order_,
                           .orderId_ = order_update.orderId_,
                           .order_update_type_ = order_update.event_type_,
                           .fill_ = std::nullopt,
                           .book_update_ = std::nullopt,
                           .execute_time_ = order_update.local_timestamp_}});
    }
    execution_engine_.clear_order_updates();
}

/**
 * @brief Order update is reflected in the local orders.
 *
 * Order updates that were processed in the exchange have not reached the
 * local system and are updated in this method after order response latency.
 */
void BacktestEngine::process_order_update_local(
    OrderEventType event_type, OrderId orderId,
    const core::trading::Order order) {
    if (event_type == OrderEventType::ACKNOWLEDGED) {
        local_active_orders_[orderId] = order;
        if (logger_) {
            logger_->log("[BacktestEngine] - " +
                             std::to_string(current_time_us_) +
                             "us - ACKNOWLEDGE recieved locally (" +
                             std::to_string(orderId) + ") update",
                         utils::logger::LogLevel::Debug);
        }
    } else if (event_type == OrderEventType::CANCELLED) {
        auto it = local_active_orders_.find(orderId);
        if (it != local_active_orders_.end()) local_active_orders_.erase(it);
        if (logger_) {
            logger_->log("[BacktestEngine] - " +
                             std::to_string(current_time_us_) +
                             "us - CANCELLED recieved locally (" +
                             std::to_string(orderId) + ") update",
                         utils::logger::LogLevel::Debug);
        }
    } else if (event_type == OrderEventType::FILL) {
        local_active_orders_[orderId] = order;
        if (logger_) {
            logger_->log("[BacktestEngine] - " +
                             std::to_string(current_time_us_) +
                             "us - FILL recieved locally (" +
                             std::to_string(orderId) + ") update",
                         utils::logger::LogLevel::Debug);
        }
    } else if (event_type == OrderEventType::REJECTED) {
        if (logger_) {
            logger_->log("[BacktestEngine] - " +
                             std::to_string(current_time_us_) +
                             "us - REJECTED recieved locally (" +
                             std::to_string(orderId) + ") update",
                         utils::logger::LogLevel::Debug);
        }
    } else {
        throw std::invalid_argument(
            "Unknown OrderEventType in local order update");
    }
}

/**
 * @brief Processes recently filled orders by scheduling them for delayed
 * handling.
 *
 * This method retrieves all fills from the execution engine, then creates a
 * corresponding `DelayedAction` for each fill with type
 * `ActionType::ProcessFill`. Each delayed action is scheduled to execute at the
 * timestamp of the fill (which typically includes response latency if modeled).
 *
 * After inserting the delayed actions into the `delayed_actions_` queue,
 * the method clears the fill buffer in the execution engine to avoid
 * processing the same fills multiple times.
 *
 * @note This method should be called regularly, such as during each elapse
 * step, to ensure fills are processed and reflected in portfolio state or PnL.
 */
void BacktestEngine::process_exchange_fills() {
    using namespace core::trading;
    using namespace core::execution_engine;
    std::vector<core::trading::Fill> fills = execution_engine_.fills();
    for (const auto &fill : fills) {
        delayed_actions_.insert(
            {fill.local_timestamp_,
             DelayedAction{.type_ = ActionType::LocalProcessFill,
                           .asset_id_ = fill.asset_id_,
                           .order_ = std::nullopt,
                           .orderId_ = std::nullopt,
                           .order_update_type_ = std::nullopt,
                           .fill_ = fill,
                           .book_update_ = std::nullopt,
                           .execute_time_ = fill.local_timestamp_}});
    }
    execution_engine_.clear_fills();
}

/**
 * @brief Updates internal accounting for a filled order.
 *
 * This method processes a single `Fill` and updates position, trading volume,
 * trading value, and realized PnL for the given asset. It is typically called
 * from delayed fill actions (e.g. in `elapse()` or `process_exchange_fills()`).
 *
 * - Position is increased for a buy and decreased for a sell.
 * - Trading volume is incremented by the fill quantity.
 * - Trading value is incremented by the quantity times price.
 * - Realized PnL is adjusted based on the fill direction and value.
 *
 * @param asset_id The ID of the asset that was filled.
 * @param fill The fill object containing execution details (side, quantity,
 * price).
 *
 * @note Assumes `fill.price_` is in quote currency. For inverse contracts,
 *       additional logic may be needed.
 */
void BacktestEngine::process_fill_local(int asset_id,
                                        const core::trading::Fill &fill) {
    using namespace core::trading;
    if (logger_) {
        logger_->log("[BacktestEngine] - " +
                         std::to_string(fill.local_timestamp_) +
                         "us - fill processed locally, price=" +
                         std::to_string(fill.price_) +
                         ", qty=" + std::to_string(fill.quantity_),
                     utils::logger::LogLevel::Debug);
    }
    Quantity signed_qty =
        (fill.side_ == TradeSide::Buy) ? fill.quantity_ : -fill.quantity_;
    local_position_[asset_id] += signed_qty;
    num_trades_[asset_id] += 1;
    trading_volume_[asset_id] += fill.quantity_;
    trading_value_[asset_id] += fill.quantity_ * fill.price_;
    double fee_rate = (fill.is_maker) ? assets_[asset_id].config().maker_fee_
                                      : assets_[asset_id].config().taker_fee_;
    double fee = fill.quantity_ * fill.price_ * fee_rate;
    local_cash_balance_ += -signed_qty * fill.price_ - fee;
}

void BacktestEngine::process_book_update_local(
    int asset_id, const core::market_data::BookUpdate &book_update) {
    using namespace core::orderbook;
    local_orderbooks_.at(asset_id).apply_book_update(book_update);
}

/**
 * @brief Returns a vector of active orders for the specified asset.
 *
 * This method retrieves all active orders for the given asset ID from the
 * local order book. It returns a vector of `Order` objects representing
 * the current state of each order.
 *
 * @param asset_id The identifier of the asset for which to retrieve orders.
 * @return A vector containing all active orders for the specified asset.
 */
const std::vector<core::trading::Order>
BacktestEngine::orders(int asset_id) const {
    if (logger_) {
        logger_->log("[BacktestEngine] - " + std::to_string(current_time_us_) +
                         "us - retrieving " +
                         std::to_string(local_active_orders_.size()) +
                         " local active orders for asset " + std::to_string(asset_id),
                     utils::logger::LogLevel::Debug);
    }
    std::vector<core::trading::Order> active_orders;
    active_orders.reserve(local_active_orders_.size());
    for (const auto &[id, order] : local_active_orders_) {
        active_orders.emplace_back(order);
    }
    return active_orders;
}

/**
 * @brief Returns the current cash balance of the backtest portfolio.
 *
 * This method retrieves the cash balance available in the backtest engine,
 * which is used for trading and position management.
 *
 * @return The current cash balance as a double.
 */

double BacktestEngine::cash() const { return local_cash_balance_; }

/**
 * @brief Returns the current equity value of the backtest portfolio.
 *
 * This method calculates the total equity by summing the cash balance and the
 * value of all local positions based on their mid prices in the order book.
 *
 * @return The total equity as a double.
 */
double BacktestEngine::equity() const {
    if (logger_) {
        logger_->log("[BacktestEngine] - " + std::to_string(current_time_us_) +
                         "us - calculating equity",
                     utils::logger::LogLevel::Debug);
    }
    double value = local_cash_balance_;
    for (auto &[asset_id, pos] : local_position_) {
        using namespace core::orderbook;
        value += pos * local_orderbooks_.at(asset_id).mid_price();
        if (logger_) {
            logger_->log(
                "[BacktestEngine] - " + std::to_string(current_time_us_) +
                    "us - asset " + std::to_string(asset_id) +
                    " position: " + std::to_string(pos) + ", mid price: " +
                    std::to_string(local_orderbooks_.at(asset_id).mid_price()),
                utils::logger::LogLevel::Debug);
        }
    }
    return value;
}

/**
 * @brief Returns the current position for the specified asset.
 *
 * This function retrieves the net position of the given asset based on
 * previously executed trades or orders. A positive value indicates a net
 * long position, while a negative value indicates a net short position.
 *
 * @param asset_id The identifier of the asset.
 * @return The current position as a double.
 */
Quantity BacktestEngine::position(int asset_id) const {
    auto it = local_position_.find(asset_id);
    return (it != local_position_.end()) ? it->second : 0.0;
}

const core::trading::Depth BacktestEngine::depth(int asset_id) const {
    using namespace core::orderbook;
    using namespace core::trading;
    Ticks best_ask =
        local_orderbooks_.at(asset_id).price_at_level(BookSide::Ask, 0);
    Ticks best_bid =
        local_orderbooks_.at(asset_id).price_at_level(BookSide::Bid, 0);
    Quantity ask_0_size =
        local_orderbooks_.at(asset_id).depth_at_level(BookSide::Ask, 0);
    Quantity bid_0_size =
        local_orderbooks_.at(asset_id).depth_at_level(BookSide::Bid, 0);
    if (logger_) {
        logger_->log("[BacktestEngine] - " + std::to_string(current_time_us_) +
                         "us - retrieving depth for asset " +
                         std::to_string(asset_id),
                     utils::logger::LogLevel::Debug);
    }
    return Depth{.best_bid_ = best_bid,
                 .bid_qty_ = bid_0_size,
                 .best_ask_ = best_ask,
                 .ask_qty_ = ask_0_size,
                 .bid_depth_ = local_orderbooks_.at(asset_id).bid_book(),
                 .ask_depth_ = local_orderbooks_.at(asset_id).ask_book(),
                 .tick_size_ = tick_sizes_.at(asset_id),
                 .lot_size_ = lot_sizes_.at(asset_id)};
}

/**
 * @brief Prints trading statistics for the specified asset.
 *
 * This method outputs the trading statistics for a given asset ID, including
 * the number of trades, total trading volume, total trading value, and
 * realized PnL. It retrieves these values from the internal maps that track
 * per-asset statistics.
 *
 * @param asset_id The identifier of the asset for which to print statistics.
 */
void BacktestEngine::print_trading_stats(int asset_id) const {
    auto num_trades_it = num_trades_.find(asset_id);
    auto trading_volume_it = trading_volume_.find(asset_id);
    auto trading_value_it = trading_value_.find(asset_id);

    std::cout << "=== Trading Statistics for : "
              << assets_.at(asset_id).config().name_
              << " ===\n";
    std::cout << "Number of Trades   : "
              << (num_trades_it != num_trades_.end() ? num_trades_it->second
                                                     : 0)
              << "\n";
    std::cout << "Trading Volume     : "
              << (trading_volume_it != trading_volume_.end()
                      ? trading_volume_it->second
                      : 0.0)
              << "\n";
    std::cout << "Trading Value      : "
              << (trading_value_it != trading_value_.end()
                      ? trading_value_it->second
                      : 0.0)
              << " USDT\n";
    std::cout << "=============================================\n";
}

/**
 * @brief Returns the current simulation time in microseconds.
 *
 * This method retrieves the internal clock of the backtest engine, which
 * represents the current simulation timestamp. The time is advanced
 * using the `elapse()` method.
 *
 * @return Current timestamp in microseconds.
 */
Timestamp BacktestEngine::current_time() const {
    return current_time_us_;
}

/**
 * @brief Sets the cash balance for the backtest portfolio.
 *
 * This method updates the internal cash balance used for trading and
 * position management. It does not affect existing positions or orders.
 *
 * @param cash The new cash balance to set (must be non-negative).
 * @throws std::invalid_argument If the provided cash amount is negative.
 */
void BacktestEngine::set_cash(double cash) {
    if (cash < 0.0) {
        throw std::invalid_argument("Cash balance cannot be negative");
    }
    local_cash_balance_ = cash;
    if (logger_) {
        logger_->log("[BacktestEngine] - " + std::to_string(current_time_us_) +
                         "us - Cash balance set to " +
                         std::to_string(local_cash_balance_),
                     utils::logger::LogLevel::Info);
    }
}

/**
 * @brief Sets the order entry latency in microseconds
 * @param Latency in microseconds
 */
void BacktestEngine::set_order_entry_latency(Microseconds latency) {
    using namespace core::execution_engine;
    order_entry_latency_us = latency;
    execution_engine_.set_order_entry_latency_us(latency);
}

/**
 * @brief Sets ther order response latency in microseconds
 * @param Latency in microseconds
 */
void BacktestEngine::set_order_response_latency(Microseconds latency) {
    using namespace core::execution_engine;
    order_response_latency_us = latency;
    execution_engine_.set_order_response_latency_us(latency);
}

/**
 * @brief Sets the market feed latency in microseconds
 * @param Latency in microseconds
 */
void BacktestEngine::set_market_feed_latency(Microseconds latency) {
    market_feed_latency_us = latency;
}

/**
 * @brief Returns the order entry latency in microseconds.
 *
 * This method retrieves the configured order entry latency, which simulates
 * the time taken for an order to be accepted by the exchange after submission.
 *
 * @return The order entry latency in microseconds.
 */
Microseconds BacktestEngine::order_entry_latency() const {
    return order_entry_latency_us;
}

/**
 * @brief Returns the order response latency in microseconds.
 *
 * This method retrieves the configured order response latency, which simulates
 * the time taken for an order update (e.g., ACK, FILL) to be reflected in the
 * local system after being processed by the exchange.
 *
 * @return The order response latency in microseconds.
 */
Microseconds BacktestEngine::order_response_latency() const {
    return order_response_latency_us;
}

/**
 * @brief Returns the market feed latency in microseconds.
 *
 * This method retrieves the configured market feed latency, which simulates
 * the time taken for market data updates (e.g., book updates, trades) to be
 * received and processed by the backtest engine.
 *
 * @return The market feed latency in microseconds.
 */
Microseconds BacktestEngine::market_feed_latency() const {
    return market_feed_latency_us;
}
} // namespace core::backtest