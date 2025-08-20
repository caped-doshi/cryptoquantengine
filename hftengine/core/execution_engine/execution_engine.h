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

#include "../../utils/logger/logger.h"
#include "../orderbook/orderbook.h"
#include "../trading/depth.h"
#include "../trading/fill.h"
#include "../trading/order.h"
#include "../trading/order_update.h"
#include "../types/order_event_type.h"
#include "../types/order_type.h"
#include "../types/time_in_force.h"
#include "../types/usings.h"

class ExecutionEngine {
  public:
    ExecutionEngine(std::shared_ptr<utils::logger::Logger> logger = nullptr);

    void add_asset(int asset_id, double tick_size, double lot_size);

    bool order_inactive(const std::shared_ptr<Order> &order);
    bool clear_inactive_orders(int asset_id);
    bool cancel_order(int asset_id, const OrderId &orderId,
                      const Timestamp &current_timestamp);

    bool order_exists(const OrderId &orderId) const;

    void execute_market_order(int asset_id, TradeSide side,
                              std::shared_ptr<Order> order);
    bool execute_fok_order(int asset_id, TradeSide side,
                           std::shared_ptr<Order> order);
    bool execute_ioc_order(int asset_id, TradeSide side,
                           std::shared_ptr<Order> order);
    bool place_maker_order(int asset_id, std::shared_ptr<Order> order);

    bool execute_order(int asset_id, TradeSide side, const Order &order);

    void handle_book_update(int asset_id, const BookUpdate &book_update);
    void handle_trade(int asset_id, const Trade &trade);

    const std::vector<OrderUpdate> &order_updates() const;
    const std::vector<Fill> &fills() const;

    void clear_fills();
    void clear_order_updates();

    constexpr double f(const double x);

    void set_order_entry_latency_us(const Microseconds latency_us);
    void set_order_response_latency_us(const Microseconds latency_us);

  private:
    Microseconds order_entry_latency_us_ = 25000;
    Microseconds order_response_latency_us_ = 10000;

    std::unordered_map<int, double> tick_sizes_;
    std::unordered_map<int, double> lot_sizes_;

    std::unordered_map<int, core::orderbook::OrderBook> orderbooks_;

    std::vector<OrderUpdate> order_updates_;
    std::vector<Fill> fills_;

    struct MakerBook {
        std::unordered_map<Ticks, std::shared_ptr<Order>> bid_orders_;
        std::unordered_map<Ticks, std::shared_ptr<Order>> ask_orders_;
    };
    std::unordered_map<int, MakerBook> maker_books_;

    std::unordered_map<int, std::vector<std::shared_ptr<Order>>> active_orders_;
    std::unordered_map<OrderId, std::shared_ptr<Order>> orders_;

    std::shared_ptr<utils::logger::Logger> logger_;
};