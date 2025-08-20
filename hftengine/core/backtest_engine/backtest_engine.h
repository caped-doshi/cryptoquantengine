/*
 * File: hftengine/core/execution_engine/backtest_engine.h
 * Description: Class structure for execution engine to take orders, fill
 * orders.
 * Author: Arvind Rathnashyam
 * Date: 2025-06-26
 * License: Proprietary
 */

#pragma once

#include <cstdint>
#include <memory>
#include <unordered_map>
#include <vector>

#include "../../utils/logger/logger.h"
#include "../data/market_data_feed.h"
#include "../execution_engine/execution_engine.h"
#include "../orderbook/orderbook.h"
#include "../trading/depth.h"
#include "../trading/fill.h"
#include "../trading/order.h"
#include "../trading/orderId_generator.h"
#include "../types/action_type.h"
#include "../types/order_status.h"
#include "../types/order_type.h"
#include "../types/time_in_force.h"
#include "../types/usings.h"
#include "backtest_asset.h"
#include "backtest_engine_config.h"

class BacktestEngine {
  public:
    explicit BacktestEngine(
        const std::unordered_map<int, AssetConfig> &asset_configs,
        const BacktestEngineConfig &engine_config,
        std::shared_ptr<utils::logger::Logger> logger = nullptr);

    // global methods
    bool elapse(std::uint64_t microseconds);

    bool order_inactive(const Order &order);
    void clear_inactive_orders();

    // local origin methods
    OrderId submit_buy_order(int asset_id, Price price, Quantity quantity,
                             TimeInForce tif, OrderType orderType);
    OrderId submit_sell_order(int asset_id, Price price, Quantity quantity,
                              TimeInForce tif, OrderType orderType);
    void cancel_order(int asset_id, OrderId orderId);

    // local state access methods
    const std::vector<Order> orders(int asset_id) const;
    const double cash() const;
    const double equity() const;
    const Quantity position(int asset_id) const;
    const Depth depth(int asset_id) const;
    const Timestamp current_time() const;

    void print_trading_stats(int asset_id) const;

    void set_cash(double cash);

    void set_order_entry_latency(Microseconds latency_us);
    void set_order_response_latency(Microseconds latency_us);
    void set_market_feed_latency(Microseconds latency_us);

    const Microseconds order_entry_latency() const;
    const Microseconds order_response_latency() const;
    const Microseconds market_feed_latency() const;

  private:
    Microseconds order_entry_latency_us = 25000;
    Microseconds order_response_latency_us = 10000;
    Microseconds market_feed_latency_us = 50000;

    // backtest simulation methods
    void process_exchange_order_updates();
    void process_exchange_fills();

    void process_order_update_local(OrderEventType event_type, OrderId orderId,
                                    Order order);
    void process_fill_local(int asset_id, const Fill &fill);
    void process_book_update_local(int asset_id, const BookUpdate &book_update);

    Timestamp current_time_us_;
    ExecutionEngine execution_engine_;
    MarketDataFeed market_data_feed_;
    OrderIdGenerator orderId_gen_;

    // asset configurations
    std::unordered_map<int, BacktestAsset> assets_;
    std::unordered_map<int, double> tick_sizes_;
    std::unordered_map<int, double> lot_sizes_;

    // local state (updated with latency simulation)
    double local_cash_balance_;
    std::unordered_map<int, double> local_position_;
    std::unordered_map<int, core::orderbook::OrderBook> local_orderbooks_;
    std::unordered_map<int, Order> local_active_orders_;

    // trading statistics
    std::unordered_map<int, int> num_trades_;
    std::unordered_map<int, double> trading_volume_;
    std::unordered_map<int, double> trading_value_;
    std::unordered_map<int, double> realized_pnl_;

    struct DelayedAction {
        ActionType type_;
        int asset_id_;
        std::optional<Order> order_;
        std::optional<OrderId> orderId_;
        std::optional<OrderEventType> order_update_type_;
        std::optional<Fill> fill_;
        std::optional<BookUpdate> book_update_;
        Timestamp execute_time_;
    };

    std::multimap<Timestamp, DelayedAction> delayed_actions_;

    std::shared_ptr<utils::logger::Logger> logger_;
};