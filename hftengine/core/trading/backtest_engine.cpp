/*
 * File: hftengine/core/trading/backtest_engine.h
 * Description: BacktestEngine to simulate trading with multiple assets.
 * Author: Arvind Rathnashyam
 * Date: 2025-06-26
 * License: Proprietary
 */

#include <cstdint>
#include <iostream>
#include <map>
#include <optional>
#include <unordered_map>

#include "../../utils/logger/logger.h"
#include "../../utils/math/math_utils.h"
#include "../data/market_data_feed.h"
#include "../types/action_type.h"
#include "../types/order_status.h"
#include "asset_config.h"
#include "backtest_asset.h"
#include "backtest_engine.h"
#include "depth.h"

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
    const std::unordered_map<int, AssetConfig> &asset_configs,
    std::shared_ptr<Logger> logger)
    : current_time_us_(0), cash_balance(0.0), logger_(logger) {

    execution_engine_ = ExecutionEngine(logger_);
    for (const auto &[asset_id, config] : asset_configs) {
        // Initialize BacktestAsset
        assets_.emplace(asset_id, BacktestAsset(config));
        // Initialize execution engine with tick and lot sizes
        execution_engine_.add_asset(asset_id, config.tick_size_,
                                    config.lot_size_);
        // Initialize MarketDataFeed streams
        market_data_feed_.add_stream(asset_id, config.book_update_file_,
                                     config.trade_file_);
        // Initialize local orderbooks
        local_orderbooks_.emplace(
            asset_id, OrderBook(config.tick_size_, config.lot_size_, logger_));
        // Initialize per-asset tracking state
        num_trades_[asset_id] = 0;
        trading_volume_[asset_id] = 0.0;
        trading_value_[asset_id] = 0.0;
        realized_pnl_[asset_id] = 0.0;
        local_position_[asset_id] = 0.0;
        // tick and lot sizes
        tick_sizes_[asset_id] = config.tick_size_;
        lot_sizes_[asset_id] = config.lot_size_;
    }
    logger_->log("[BacktestEngine] - Initialized with " +
                 std::to_string(assets_.size()) + " assets.");
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
    return true;
}


bool BacktestEngine::order_inactive(const Order &order) {
    // Check if order is filled, cancelled, or expired
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
    logger_->log("[BacktestEngine] - " + std::to_string(current_time_us_) +
                 "us - clearing inactive orders");
    for (auto it = local_active_orders_.begin();
        it != local_active_orders_.end();) {
        if (order_inactive(it->second)) {
            logger_->log("[BacktestEngine] - " + std::to_string(current_time_us_) +
                         "us - clearing inactive order (" +
                         std::to_string(it->second.orderId_) + ")");
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
    logger_->log("[BacktestEngine] - " + std::to_string(current_time_us_) +
                 "us - buy order (" + std::to_string(buy_order.orderId_) +
                 ") submitted to exchange");
    delayed_actions_.insert(
        {buy_order.exch_timestamp_,
         DelayedAction{.type_ = ActionType::SubmitBuy,
                       .asset_id_ = asset_id,
                       .order_ = buy_order,
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
    logger_->log("[BacktestEngine] - " + std::to_string(current_time_us_) +
                 "us - sell order (" + std::to_string(sell_order.orderId_) +
                 ") submitted to exchange");
    delayed_actions_.insert(
        {sell_order.exch_timestamp_,
         DelayedAction{.type_ = ActionType::SubmitSell,
                       .asset_id_ = asset_id,
                       .order_ = sell_order,
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
                       .orderId_ = orderId,
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
    std::vector<OrderUpdate> order_updates = execution_engine_.order_updates();
    for (const auto &order_update : order_updates) {
        delayed_actions_.insert(
            {order_update.local_timestamp_,
             DelayedAction{.type_ = ActionType::LocalOrderUpdate,
                           .asset_id_ = order_update.asset_id_,
                           .order_ = *order_update.order_,
                           .orderId_ = order_update.orderId_,
                           .order_update_type_ = order_update.event_type_,
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
void BacktestEngine::process_order_update_local(OrderEventType event_type,
                                                OrderId orderId, Order order) {
    if (event_type == OrderEventType::ACKNOWLEDGED) {
        local_active_orders_[orderId] = order;
        logger_->log("[BacktestEngine] - " + std::to_string(current_time_us_) +
                     "us - ACKNOWLEDGE recieved locally (" +
                     std::to_string(orderId) + ") update");
    } else if (event_type == OrderEventType::CANCELLED) {
        auto it = local_active_orders_.find(orderId);
        if (it != local_active_orders_.end()) local_active_orders_.erase(it);
        logger_->log("[BacktestEngine] - " + std::to_string(current_time_us_) +
                     "us - CANCELLED recieved locally (" +
                     std::to_string(orderId) + ") update");
    } else if (event_type == OrderEventType::FILL) {
        /* if (order.filled_quantity_ == order.quantity_) {
            auto it = local_active_orders_.find(orderId);
            if (it != local_active_orders_.end())
                local_active_orders_.erase(it);
        } else {
            local_active_orders_[orderId] = order;
        }*/
        local_active_orders_[orderId] = order;
        logger_->log("[BacktestEngine] - " + std::to_string(current_time_us_) +
                     "us - FILL recieved locally (" + std::to_string(orderId) +
                     ") update");
    } else if (event_type == OrderEventType::REJECTED) {
        logger_->log("[BacktestEngine] - " + std::to_string(current_time_us_) +
                     "us - REJECTED recieved locally (" +
                     std::to_string(orderId) + ") update");
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
    std::vector<Fill> fills = execution_engine_.fills();
    for (const auto &fill : fills) {
        delayed_actions_.insert(
            {fill.local_timestamp_,
             DelayedAction{.type_ = ActionType::LocalProcessFill,
                           .asset_id_ = fill.asset_id_,
                           .fill_ = fill,
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
void BacktestEngine::process_fill_local(int asset_id, const Fill &fill) {
    // update output to include fill price and quantity
    logger_->log("[BacktestEngine] - " + std::to_string(fill.local_timestamp_) +
                 "us - fill processed locally at price " +
                 std::to_string(fill.price_) + " @ " +
                 std::to_string(fill.quantity_) + " quantity");
    Quantity signed_qty =
        (fill.side_ == TradeSide::Buy) ? fill.quantity_ : -fill.quantity_;
    local_position_[asset_id] += signed_qty;
    trading_volume_[asset_id] += fill.quantity_;
    trading_value_[asset_id] += fill.quantity_ * fill.price_;
    double fee_rate = (fill.is_maker) ? assets_[asset_id].config().maker_fee_
                                      : assets_[asset_id].config().taker_fee_;
    double fee = fill.quantity_ * fill.price_ * fee_rate;
    cash_balance += -signed_qty * fill.price_ - fee;
}

void BacktestEngine::process_book_update_local(int asset_id,
                                               const BookUpdate &book_update) {
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
const std::vector<Order> BacktestEngine::orders(int asset_id) const {
    logger_->log("[BacktestEngine] - " + std::to_string(current_time_us_) +
                 "us - retrieving " +
                 std::to_string(local_active_orders_.size()) +
                 " local active orders");
    std::vector<Order> active_orders;
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
const double BacktestEngine::cash() const { return cash_balance; }

/**
 * @brief Returns the current equity value of the backtest portfolio.
 *
 * This method calculates the total equity by summing the cash balance and the
 * value of all local positions based on their mid prices in the order book.
 *
 * @return The total equity as a double.
 */
const double BacktestEngine::equity() const {
    double value = cash_balance;
    for (auto &[asset_id, pos] : local_position_) {
        value += pos * local_orderbooks_.at(asset_id).mid_price();
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
const Quantity BacktestEngine::position(int asset_id) const {
    auto it = local_position_.find(asset_id);
    return (it != local_position_.end()) ? it->second : 0.0;
}

const Depth BacktestEngine::depth(int asset_id) const {
    Ticks best_ask =
        local_orderbooks_.at(asset_id).price_at_level(BookSide::Ask, 0);
    Ticks best_bid =
        local_orderbooks_.at(asset_id).price_at_level(BookSide::Bid, 0);
    Quantity ask_0_size =
        local_orderbooks_.at(asset_id).depth_at_level(BookSide::Ask, 0);
    Quantity bid_0_size =
        local_orderbooks_.at(asset_id).depth_at_level(BookSide::Bid, 0);
    logger_->log("[BacktestEngine] - " + std::to_string(current_time_us_) +
                 "us - retrieving depth for asset " + std::to_string(asset_id));
    local_orderbooks_.at(asset_id).print_top_levels();
    std::unordered_map<Ticks, Quantity> bid_depth;
    int bid_levels = local_orderbooks_.at(asset_id).bid_levels();
    for (int level = 0; level < bid_levels; level++) {
        Ticks bid_level_price_ticks =
            local_orderbooks_.at(asset_id).price_at_level(BookSide::Bid, level);
        double bid_level_depth =
            local_orderbooks_.at(asset_id).depth_at_level(BookSide::Bid, level);
        bid_depth[bid_level_price_ticks] = bid_level_depth;
    }
    std::unordered_map<Ticks, Quantity> ask_depth;
    int ask_levels = local_orderbooks_.at(asset_id).ask_levels();
    for (int level = 0; level < ask_levels; level++) {
        Ticks ask_level_price_ticks =
            local_orderbooks_.at(asset_id).price_at_level(BookSide::Ask, level);
        double ask_level_depth =
            local_orderbooks_.at(asset_id).depth_at_level(BookSide::Ask, level);
        ask_depth[ask_level_price_ticks] = ask_level_depth;
    }
    return Depth{.best_bid_ = best_bid,
                 .bid_qty_ = bid_0_size,
                 .best_ask_ = best_ask,
                 .ask_qty_ = ask_0_size,
                 .bid_depth_ = bid_depth,
                 .ask_depth_ = ask_depth,
                 .tick_size_ = tick_sizes_.at(asset_id),
                 .lot_size_ = lot_sizes_.at(asset_id)};
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
const Timestamp BacktestEngine::current_time() const {
    return current_time_us_;
}

/**
 * @brief Sets the order entry latency in microseconds
 * @param Latency in microseconds
 */
void BacktestEngine::set_order_entry_latency(Microseconds latency) {
    order_entry_latency_us = latency;
    execution_engine_.set_order_entry_latency_us(latency);
}

/**
 * @brief Sets ther order response latency in microseconds
 * @param Latency in microseconds
 */
void BacktestEngine::set_order_response_latency(Microseconds latency) {
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
const Microseconds BacktestEngine::order_entry_latency() const {
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
const Microseconds BacktestEngine::order_response_latency() const {
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
const Microseconds BacktestEngine::market_feed_latency() const {
    return market_feed_latency_us;
}