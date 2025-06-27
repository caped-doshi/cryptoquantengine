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

#include "../orderbook/orderbook.h"
#include "../trading/order.h"
#include "../types/order_type.h"
#include "../types/time_in_force.h"
#include "../types/usings.h"
#include "backtest_asset.h"

class BacktestEngine {
  public:
    bool elapse(std::uint64_t microseconds);

    bool submit_buy_order(int asset_id, OrderId orderId, Price price,
                          Quantity quantity, TimeInForce tif,
                          OrderType orderType);

    bool submit_sell_order(int asset_id, OrderId orderId, Price price,
                           Quantity quantity, TimeInForce tif,
                           OrderType orderType);

    void cancel(int asset_id, OrderId orderID);

  private:
    BacktestAsset asset_;
    Timestamp current_time_us_;

    OrderBook orderbook_;
    std::unordered_map<OrderId, Order> orders_;
};