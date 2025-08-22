/*
 * Copyright (c) 2025 arvindkrv@protonmail.com
 *
 * Please see the LICENSE file for the terms and conditions
 * associated with this software.
 */

#include <cmath>
#include <iostream>
#include <memory>
#include <unordered_map>
#include <unordered_set>
#include <vector>

#include "../../../utils/logger/log_level.h"
#include "../../../utils/logger/logger.h"
#include "../../../utils/math/math_utils.h"
#include "../../backtest_engine/backtest_engine.h"
#include "../../trading/depth.h"
#include "../../trading/order.h"
#include "../../types/aliases/usings.h"
#include "../../types/enums/book_side.h"
#include "../../types/enums/order_status.h"
#include "../strategy.h"
#include "grid_trading.h"
#include "grid_trading_config.h"

namespace core::strategy {
GridTrading::GridTrading(int asset_id, int grid_num, Ticks grid_interval,
                         Ticks half_spread, double position_limit,
                         double notional_order_qty,
                         std::shared_ptr<utils::logger::Logger> logger)
    : asset_id_(asset_id), grid_num_(grid_num), grid_interval_(grid_interval),
      half_spread_(half_spread), position_limit_(position_limit),
      notional_order_qty_(notional_order_qty), logger_(logger) {
    initialize();
}

GridTrading::GridTrading(int asset_id,
                         const core::strategy::GridTradingConfig &config,
                         std::shared_ptr<utils::logger::Logger> logger)
    : asset_id_(asset_id), grid_num_(config.grid_num_),
      grid_interval_(config.grid_interval_), half_spread_(config.half_spread_),
      position_limit_(config.position_limit_),
      notional_order_qty_(config.notional_order_qty_), logger_(logger) {
    initialize();
}

void GridTrading::initialize() {
    if (logger_) {
        logger_->log("[GridTrading] - Strategy initialized for asset ID: " +
                         std::to_string(asset_id_),
                     utils::logger::LogLevel::Debug);
    }
}

void GridTrading::on_elapse(core::backtest::BacktestEngine &engine) {
    using namespace core::trading;
    Depth depth = engine.depth(asset_id_);
    Quantity position = engine.position(asset_id_);
    const std::vector<Order> orders = engine.orders(asset_id_);

    double tick_size = depth.tick_size_;
    double lot_size = depth.lot_size_;
    Price best_bid = utils::math::ticks_to_price(depth.best_bid_, tick_size);
    Price best_ask = utils::math::ticks_to_price(depth.best_ask_, tick_size);

    if (best_bid <= 0.0 || best_ask <= 0.0 || !std::isfinite(best_bid) ||
        !std::isfinite(best_ask)) {
        if (logger_) {
            logger_->log("[GridTrading] - Skipping grid setup: invalid bid/ask "
                         "prices for asset ID: " +
                             std::to_string(asset_id_) +
                             " (bid=" + std::to_string(best_bid) +
                             ", ask=" + std::to_string(best_ask) + ")",
                         utils::logger::LogLevel::Debug);
        }
        return;
    }

    Price mid_price = (best_bid + best_ask) / 2.0;

    Price bid_price = std::floor((mid_price - half_spread_ * tick_size) /
                                 (grid_interval_ * tick_size)) *
                      grid_interval_ * tick_size;
    Price ask_price = std::ceil((mid_price + half_spread_ * tick_size) /
                                (grid_interval_ * tick_size)) *
                      grid_interval_ * tick_size;

    // Create new bid and ask order grids
    std::unordered_set<Ticks> new_bid_prices;
    if (position < position_limit_) {
        for (int i = 0; i < grid_num_; i++) {
            const Ticks bid_price_ticks =
                static_cast<Ticks>(std::floor(bid_price / tick_size));
            new_bid_prices.insert(bid_price_ticks);
            bid_price -= grid_interval_ * tick_size;
        }
    }
    std::unordered_set<Ticks> new_ask_prices;
    if (position > -position_limit_) {
        for (int i = 0; i < grid_num_; ++i) {
            const Ticks ask_price_ticks =
                static_cast<Ticks>(std::ceil(ask_price / tick_size));
            new_ask_prices.insert(ask_price_ticks);
            ask_price += grid_interval_ * tick_size;
        }
    }
    // Cancel orders not in the new grid
    std::unordered_set<Ticks> existing_bid_prices;
    std::unordered_set<Ticks> existing_ask_prices;
    for (const Order &order : orders) {
        if (order.orderStatus_ == OrderStatus::ACTIVE ||
            order.orderStatus_ == OrderStatus::PARTIALLY_FILLED) {
            Ticks order_price_ticks =
                (order.side_ == BookSide::Bid)
                    ? static_cast<Ticks>(std::floor(order.price_ / tick_size))
                    : static_cast<Ticks>(std::ceil(order.price_ / tick_size));
            // add to existing prices
            if (order.side_ == BookSide::Bid) {
                existing_bid_prices.insert(order_price_ticks);
            } else {
                existing_ask_prices.insert(order_price_ticks);
            }
            // cancel orders not in the new grid
            if ((order.side_ == BookSide::Bid &&
                 new_bid_prices.find(order_price_ticks) ==
                     new_bid_prices.end()) ||
                (order.side_ == BookSide::Ask &&
                 new_ask_prices.find(order_price_ticks) ==
                     new_ask_prices.end())) {
                engine.cancel_order(asset_id_, order.orderId_);
                if (logger_) {
                    if (order.side_ == BookSide::Bid) {
                        logger_->log(
                            "[GridTrading] - Cancelled bid order at price: " +
                                std::to_string(order.price_) +
                                " for asset ID: " + std::to_string(asset_id_),
                            utils::logger::LogLevel::Debug);
                    } else {
                        logger_->log(
                            "[GridTrading] - Cancelled ask order at price: " +
                                std::to_string(order.price_) +
                                " for asset ID: " + std::to_string(asset_id_),
                            utils::logger::LogLevel::Debug);
                    }
                }
            }
        }
    }
    // submit new orders for the grid
    double raw_qty = notional_order_qty_ / mid_price;
    Quantity order_qty = std::round(raw_qty / lot_size) * lot_size;
    for (const Ticks &bid_price_ticks : new_bid_prices) {
        if (existing_bid_prices.find(bid_price_ticks) ==
            existing_bid_prices.end()) {
            Price bid_price = bid_price_ticks * tick_size;
            if (bid_price_ticks <= 0) {
                if (logger_) {
                    logger_->log(
                        "[GridTrading] - Invalid bid price: " +
                            std::to_string(bid_price) +
                            " for asset ID: " + std::to_string(asset_id_) +
                            ". Skipping order submission.",
                        utils::logger::LogLevel::Info);
                }
                continue;
            }
            if (order_qty <= 0.0) {
                if (logger_) {
                    logger_->log(
                        "[GridTrading] - Invalid bid order quantity: " +
                            std::to_string(order_qty) +
                            " for asset ID: " + std::to_string(asset_id_) +
                            ". Skipping order submission.",
                        utils::logger::LogLevel::Info);
                }
                continue;
            }
            if (logger_) {
                logger_->log("[GridTrading] - Submitted buy order : asset_id=" +
                                 std::to_string(asset_id_) +
                                 ", price=" + std::to_string(bid_price) +
                                 ", qty=" + std::to_string(order_qty),
                             utils::logger::LogLevel::Info);
            }
            engine.submit_buy_order(asset_id_, bid_price, order_qty,
                                 TimeInForce::GTC, OrderType::LIMIT);
        }
    }
    for (const Ticks &ask_price_ticks : new_ask_prices) {
        if (existing_ask_prices.find(ask_price_ticks) ==
            existing_ask_prices.end()) {
            Price ask_price = ask_price_ticks * tick_size;
            if (ask_price <= 0.0) {
                if (logger_) {
                    logger_->log(
                        "[GridTrading] - Invalid ask price: " +
                            std::to_string(bid_price) +
                            " for asset ID: " + std::to_string(asset_id_) +
                            ". Skipping order submission.",
                        utils::logger::LogLevel::Info);
                }
                continue;
            }
            if (order_qty <= 0.0) {
                if (logger_) {
                    logger_->log(
                        "[GridTrading] - Invalid ask order quantity: " +
                            std::to_string(order_qty) +
                            " for asset ID: " + std::to_string(asset_id_) +
                            ". Skipping order submission.",
                        utils::logger::LogLevel::Info);
                }
                continue;
            }
            engine.submit_sell_order(asset_id_, ask_price, order_qty,
                                  TimeInForce::GTC, OrderType::LIMIT);
            if (logger_) {
                logger_->log("[GridTrading] - Submitted buy order : asset_id=" +
                                 std::to_string(asset_id_) +
                                 ", price=" + std::to_string(bid_price) +
                                 ", qty=" + std::to_string(order_qty),
                             utils::logger::LogLevel::Info);
            }
        }
    }
}
} // namespace core::strategy