/*
 * File: hftengine/core/execution_engine/execution_engine.h
 * Description: Class structure for execution engine to take orders, fill.
 * orders.
 * Author: Arvind Rathnashyam
 * Date: 2025-06-26
 * License: Proprietary
 */

#pragma once

#include <unordered_map>
#include <vector>

#include "../orderbook/orderbook.h"
#include "../trading/fill.h"
#include "../trading/order.h"
#include "../types/order_type.h"
#include "../types/time_in_force.h"
#include "../types/usings.h"

class ExecutionEngine {
  public:
    ExecutionEngine();
    bool submit_buy_order(int asset_id, const Timestamp &timestamp,
                          const OrderId &orderId, const Price &price,
                          const Quantity &quantity, const TimeInForce &tif,
                          const OrderType &orderType);
    bool submit_sell_order(int asset_id, const Timestamp &timestamp,
                           const OrderId &orderId, const Price &price,
                           const Quantity &quantity, const TimeInForce &tif,
                           const OrderType &orderType);
    bool clear_inactive_orders(int asset_id);
    bool cancel_order(const OrderId &orderId);
    std::vector<Order> orders(int asset_id) const;

    void handle_trade(int asset_id, const Trade &trade);
    void handle_l2update(int asset_id, const L2Update &l2_update);

    std::vector<Fill> match_orders(int asset_id) const;

  private:
    OrderBook orderbook_;
    std::vector<Fill> fills_;

    std::unordered_map<OrderId, Order> bid_orders_;
    std::unordered_map<OrderId, Order> ask_orders_;
};