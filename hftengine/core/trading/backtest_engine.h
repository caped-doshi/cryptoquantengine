/*
 * File: hftengine/core/execution_engine/backtest_engine.h
 * Description: Class structure for execution engine to take orders, fill
 * orders. Author: Arvind Rathnashyam
 * Date: 2025-06-26
 * License: Proprietary
 */

#pragma once

#include <cstdint>
#include <unordered_map>
#include <vector>
#include <memory>

#include "../data/readers/market_data_feed.hpp"
#include "../execution_engine/execution_engine.h"
#include "../orderbook/orderbook.h"
#include "../trading/order.h"
#include "../types/order_type.h"
#include "../types/time_in_force.h"
#include "../types/usings.h"
#include "backtest_asset.h"
#include "depth.h"

class BacktestEngine {
  public:
    BacktestEngine();

    bool elapse(std::uint64_t microseconds);
    bool submit_buy_order(int asset_id, const OrderId &orderId,
                          const Price &price, const Quantity &quantity,
                          const TimeInForce &tif, const OrderType &orderType);
    bool submit_sell_order(int asset_id, const OrderId &orderId,
                           const Price &price, const Quantity &quantity,
                           const TimeInForce &tif, const OrderType &orderType);
    bool cancel_order(int asset_id, const OrderId &orderId);
    bool clear_inactive_orders(int asset_id);
    std::vector<Order> orders(int asset_id);
    Quantity position(int asset_id);
    Depth depth(int asset_id);

  private:
    Timestamp current_time_us_; // microseconds
    ExecutionEngine execution_engine_;
    MarketDataFeed market_data_feed_;
    std::unordered_map<int, BacktestAsset> assets_;

    // Per-asset state
    double balance;
    std::unordered_map<int, int> num_trades_;
    std::unordered_map<int, double> trading_volume_;
    std::unordered_map<int, double> trading_value_;
    std::unordered_map<int, double> realized_pnl_;
    std::unordered_map<int, double> position_;
};