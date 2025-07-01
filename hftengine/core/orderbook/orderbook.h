/*
 * File: hftengine/core/orderbook/orderbook.h
 * Description: 
 * Author: Arvind Rathnashyam
 * Date: 2025-06-23
 * License: Proprietary
 */

# pragma once

# include <map>

# include "../types/usings.h"
# include "../types/trade_side.h"
# include "../types/book_side.h"
# include "../types/update_type.h"
# include "../market_data/book_update.h"
# include "../market_data/trade.h"

#pragma once

#include <map>
#include <unordered_map>
#include <vector>
#include <string>
#include <cstdint>
#include <limits>
#include <optional>

class OrderBook {
public:
    // Constructor
    OrderBook();

    // Apply incoming L2 book update (price-level based)
    void apply_book_update(const BookUpdate& update);

    // Apply trade message to simulate fills
    void apply_trade(const Trade& trade);

    // Book queries
    Price best_bid() const;
    Price best_ask() const;
    Price mid_price() const;

    Quantity depth_at(BookSide side, Price price) const;
    Quantity depth_at_level(BookSide side, int level) const;
    Price price_at_level(BookSide side, int level) const;

    int bid_levels() const;
    int ask_levels() const;

    // Clear book (e.g., on snapshot)
    void clear();

    // debugging and testing
    void print_top_levels(int depth = 5) const;
    bool is_empty() const;

private:
    std::map<double, double, std::greater<>> bid_book_;  // price -> qty
    std::map<double, double> ask_book_;                  // price -> qty
    UpdateType last_update_;

    // Helper to update queue position estimates
    void update_queue_positions(const Trade& trade);
};
