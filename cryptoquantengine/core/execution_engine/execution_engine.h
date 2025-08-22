/*
 * Copyright (c) 2025 arvindkrv@protonmail.com
 *
 * Please see the LICENSE file for the terms and conditions
 * associated with this software.
 */

#pragma once

#include <deque>
#include <functional>
#include <memory>
#include <type_traits>
#include <unordered_map>
#include <vector>

#include "../../utils/logger/logger.h"
#include "../orderbook/orderbook.h"
#include "../trading/depth.h"
#include "../trading/fill.h"
#include "../trading/order.h"
#include "../trading/order_update.h"
#include "../types/enums/order_event_type.h"
#include "../types/enums/order_type.h"
#include "../types/enums/time_in_force.h"
#include "../types/aliases/usings.h"

namespace core::execution_engine {
class ExecutionEngine {
  public:
    ExecutionEngine(std::shared_ptr<utils::logger::Logger> logger = nullptr);

    void add_asset(int asset_id, double tick_size, double lot_size);

    bool order_inactive(const std::shared_ptr<core::trading::Order> &order);
    bool clear_inactive_orders(int asset_id);
    bool cancel_order(int asset_id, const OrderId &orderId,
                      const Timestamp &current_timestamp);

    bool order_exists(const OrderId &orderId) const;

    void execute_market_order(int asset_id, TradeSide side,
                              std::shared_ptr<core::trading::Order> order);
    bool execute_fok_order(int asset_id, TradeSide side,
                           std::shared_ptr<core::trading::Order> order);
    bool execute_ioc_order(int asset_id, TradeSide side,
                           std::shared_ptr<core::trading::Order> order);
    bool place_maker_order(int asset_id,
                           std::shared_ptr<core::trading::Order> order);

    bool execute_order(int asset_id, TradeSide side,
                       const core::trading::Order &order);

    void handle_book_update(int asset_id, const core::market_data::BookUpdate &book_update);
    void handle_trade(int asset_id, const core::market_data::Trade &trade);

    const std::vector<core::trading::OrderUpdate> &order_updates() const;
    const std::vector<core::trading::Fill> &fills() const;

    void clear_fills();
    void clear_order_updates();

    constexpr double f(double x);

    void set_order_entry_latency_us(const Microseconds latency_us);
    void set_order_response_latency_us(const Microseconds latency_us);

  private:
    Microseconds order_entry_latency_us_ = 25000;
    Microseconds order_response_latency_us_ = 10000;

    std::unordered_map<int, double> tick_sizes_;
    std::unordered_map<int, double> lot_sizes_;

    std::unordered_map<int, core::orderbook::OrderBook> orderbooks_;

    std::vector<core::trading::OrderUpdate> order_updates_;
    std::vector<core::trading::Fill> fills_;

    struct MakerBook {
        std::unordered_map<Ticks, std::shared_ptr<core::trading::Order>>
            bid_orders_;
        std::unordered_map<Ticks, std::shared_ptr<core::trading::Order>>
            ask_orders_;
    };
    std::unordered_map<int, MakerBook> maker_books_;

    std::unordered_map<int, std::vector<std::shared_ptr<core::trading::Order>>>
        active_orders_;
    std::unordered_map<OrderId, std::shared_ptr<core::trading::Order>> orders_;

    std::shared_ptr<utils::logger::Logger> logger_;

    template <typename Container>
    static void clear_from_container(
        Container &container,
        const std::function<bool(const std::shared_ptr<core::trading::Order> &)>
            &order_inactive);
};
}