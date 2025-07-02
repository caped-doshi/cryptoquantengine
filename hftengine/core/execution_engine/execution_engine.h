/*
 * File: hftengine/core/execution_engine/execution_engine.h
 * Description: Class structure for execution engine to take orders, fill.
 * orders.
 * Author: Arvind Rathnashyam
 * Date: 2025-06-26
 * License: Proprietary
 */

#pragma once

#include <deque>
#include <memory>
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

    bool clear_inactive_orders(int asset_id);
    bool cancel_order(const OrderId &orderId);

    void execute_market_order(int asset_id, TradeSide side,
                              std::shared_ptr<Order> order);
    bool execute_fok_order(int asset_id, TradeSide side,
                           std::shared_ptr<Order> order);
    bool execute_ioc_order(int asset_id, TradeSide side,
                           std::shared_ptr<Order> order);
    bool place_gtx_order(int asset_id, std::shared_ptr<Order> order);

    void handle_book_update(int asset_id, const BookUpdate &book_update);
    void handle_trade(int asset_id, const Trade &trade);

    std::vector<Fill> match_orders(int asset_id) const;

    std::vector<Order> orders(int asset_id) const;
    const std::vector<Fill> &fills() const;

    void clear_fills();
    double f(const double x);

  private:
    std::map<int, OrderBook> orderbooks_;
    std::vector<Fill> fills_;

    // one order per price level on each side
    std::map<Price, std::shared_ptr<Order>, std::greater<>> bid_orders_;
    std::map<Price, std::shared_ptr<Order>> ask_orders_;

    std::unordered_map<int, std::vector<std::shared_ptr<Order>>> active_orders_;
    std::unordered_map<OrderId, std::shared_ptr<Order>> orders_;
};