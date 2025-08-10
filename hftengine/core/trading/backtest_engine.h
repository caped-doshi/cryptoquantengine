/*
 * File: hftengine/core/execution_engine/backtest_engine.h
 * Description: Class structure for execution engine to take orders, fill
 * orders. Author: Arvind Rathnashyam
 * Date: 2025-06-26
 * License: Proprietary
 */

#pragma once

#include <cstdint>
#include <memory>
#include <unordered_map>
#include <vector>

#include "../data/market_data_feed.h"
#include "../execution_engine/execution_engine.h"
#include "../orderbook/orderbook.h"
#include "../trading/order.h"
#include "../trading/fill.h"
#include "../types/action_type.h"
#include "../types/order_type.h"
#include "../types/time_in_force.h"
#include "../types/usings.h"
#include "backtest_asset.h"
#include "depth.h"
#include "orderId_generator.h"

class BacktestEngine {
  public:
    explicit BacktestEngine(
        const std::unordered_map<int, AssetConfig> &asset_configs);

    bool elapse(std::uint64_t microseconds);

    // local origin methods
    OrderId submit_buy_order(int asset_id, const Price &price,
                             const Quantity &quantity, const TimeInForce &tif,
                             const OrderType &orderType);
    OrderId submit_sell_order(int asset_id, const Price &price,
                              const Quantity &quantity, const TimeInForce &tif,
                              const OrderType &orderType);
    void cancel_order(int asset_id, const OrderId &orderId);

    // local state access methods
    std::vector<Order> orders(int asset_id);
    double cash();
    double equity();
    Quantity position(int asset_id);
    Depth depth(int asset_id);
    Timestamp current_time();

    const Microseconds order_entry_latency() const;
    const Microseconds order_response_latency() const;

  private:
    Microseconds order_entry_latency_us = 1000;
    Microseconds order_response_latency_us = 1000;

    // exchange origin methods
    bool clear_inactive_orders(int asset_id);

    void process_exchange_order_updates();
    void process_order_update_local(OrderEventType event_type, OrderId orderId,
                                    Order order);

    void process_exchange_fills();
    void process_fill_local(int asset_id, const Fill &fill);

    void process_book_update_local(int asset_id, const BookUpdate& book_update);

    Timestamp current_time_us_; // microseconds
    ExecutionEngine execution_engine_;
    MarketDataFeed market_data_feed_;
    OrderIdGenerator orderId_gen_;
    std::unordered_map<int, OrderBook> local_orderbooks_;
    std::unordered_map<int, Order> local_active_orders_;
    std::unordered_map<int, BacktestAsset> assets_;

    double cash_balance;
    std::unordered_map<int, int> num_trades_;
    std::unordered_map<int, double> trading_volume_;
    std::unordered_map<int, double> trading_value_;
    std::unordered_map<int, double> realized_pnl_;

    std::unordered_map<int, double> local_position_;

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
};