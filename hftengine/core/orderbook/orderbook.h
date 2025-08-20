/*
 * File: hftengine/core/orderbook/orderbook.h
 * Description:
 * Author: Arvind Rathnashyam
 * Date: 2025-06-23
 * License: Proprietary
 */

#pragma once

#include <cstdint>
#include <limits>
#include <map>
#include <memory>
#include <optional>
#include <string>
#include <unordered_map>
#include <vector>

#include "../../utils/logger/logger.h"
#include "../market_data/book_update.h"
#include "../market_data/trade.h"
#include "../types/enums/book_side.h"
#include "../types/enums/trade_side.h"
#include "../types/enums/update_type.h"
#include "../types/aliases/usings.h"

namespace core::orderbook {
class OrderBook {
  public:
    OrderBook(double tick_size, double lot_size,
              std::shared_ptr<utils::logger::Logger> logger = nullptr);

    void apply_book_update(const core::market_data::BookUpdate &update);

    Price best_bid() const;
    Price best_ask() const;
    Price mid_price() const;

    Quantity depth_at(const BookSide side, const Ticks price) const;
    Quantity depth_at_level(const BookSide side, const int level) const;
    Ticks price_at_level(const BookSide side, const int level) const;

    int bid_levels() const;
    int ask_levels() const;

    std::map<Ticks, Quantity, std::greater<>> bid_book() const;
    std::map<Ticks, Quantity> ask_book() const;

    void clear();

    void print_top_levels(int depth = 5) const;
    bool is_empty() const;

  private:
    double tick_size_;
    double lot_size_;
    std::map<Ticks, Quantity, std::greater<>> bid_book_;
    std::map<Ticks, Quantity> ask_book_;
    UpdateType last_update_;

    std::shared_ptr<utils::logger::Logger> logger_;
};
} 