/*
 * File: hftengine/core/orderbook/orderbook.cpp
 * Description:
 * Author: Arvind Rathnashyam
 * Date: 2025-06-24
 * License: Proprietary
 */

#include <algorithm>
#include <cmath>
#include <iostream>

#include "../../utils/math/math_utils.h"
#include "../market_data/book_update.h"
#include "../market_data/trade.h"
#include "../types/book_side.h"
#include "orderbook.h"

OrderBook::OrderBook() : last_update_(UpdateType::Snapshot) {}

OrderBook::OrderBook(double tick_size, double lot_size)
    : tick_size_(tick_size), lot_size_(lot_size),
      last_update_(UpdateType::Snapshot) {
    if (tick_size <= 0.0) {
        throw std::invalid_argument("Tick size must be positive: " +
                                    std::to_string(tick_size));
    }
    if (lot_size <= 0.0) {
        throw std::invalid_argument("Lot size must be positive: " +
                                    std::to_string(lot_size));
    }
}

/**
 * @brief Clears the order book by removing all bids and asks.
 *
 */
void OrderBook::clear() {
    bid_book_.clear();
    ask_book_.clear();
}

/**
 * @brief Applies a book update to the order book.
 *
 * This method updates the order book with a new book update, which can be a
 * snapshot or an incremental update. It handles both bid and ask sides of the
 * book and ensures that the price and quantity are valid.
 *
 * @param update The book update to apply.
 * @throws std::invalid_argument if the price is not positive or quantity is
 * negative.
 */
void OrderBook::apply_book_update(const BookUpdate &update) {

    if (update.price_ <= 0.0) {
        throw std::invalid_argument("Price must be positive: " +
                                    std::to_string(update.price_));
    }
    if (update.quantity_ < 0.0) {
        throw std::invalid_argument("Quantity cannot be negative: " +
                                    std::to_string(update.quantity_));
    }
    if (update.update_type_ == UpdateType::Snapshot &&
        last_update_ == UpdateType::Incremental) {
        clear();
    }
    Ticks price_ticks = price_to_ticks(update.price_, tick_size_);
    if (update.quantity_ == 0.0) {
        (update.side_ == BookSide::Bid) ? bid_book_.erase(price_ticks)
                                        : ask_book_.erase(price_ticks);
    } else {
        (update.side_ == BookSide::Bid)
            ? bid_book_[price_ticks] = update.quantity_
            : ask_book_[price_ticks] = update.quantity_;
    }
    last_update_ = update.update_type_;
}

/**
 * @brief Returns the best (highest) bid price currently in the bid book.
 *
 * @return The best bid price, or 0.0 if the bid book is empty.
 */
Price OrderBook::best_bid() const {
    return bid_book_.empty()
               ? 0.0
               : ticks_to_price(bid_book_.begin()->first, tick_size_);
}

/**
 * @brief Returns the best (lowest) ask price currently in the ask book.
 *
 * @return The best ask price, or 0.0 if the ask book is empty.
 */
Price OrderBook::best_ask() const {
    return ask_book_.empty()
               ? 0.0
               : ticks_to_price(ask_book_.begin()->first, tick_size_);
}

/**
 * @brief Returns the current mid price.
 *
 * Mid price is calculated (best bid + best ask) / 2.0
 *
 * @return The mid price, or 0.0 if the ask book or bid book is empty.
 */
Price OrderBook::mid_price() const {
    if (bid_book_.empty() || ask_book_.empty()) return 0.0;
    return (best_bid() + best_ask()) / 2.0;
}

/**
 * @brief Returns the total quantity at the given price on one side of the
 * book.
 *
 * Levels are 0-based:
 *   - For bids: level 0 = highest bid price, level 1 = second-highest, etc.
 *   - For asks: level 0 = lowest ask price, level 1 = second-lowest, etc.
 *
 * If `level` is negative or exceeds the number of price levels, this returns 0.
 * @param side  Which side of the book to query (BookSide::Bid or BookSide::Ask)
 * @param price Price level
 * @return The quantity at the price level, or 0 if it does not exist
 */
Quantity OrderBook::depth_at(const BookSide side, const Ticks price) const {
    auto it =
        (side == BookSide::Bid) ? bid_book_.find(price) : ask_book_.find(price);
    return (side == BookSide::Bid)
               ? ((it != bid_book_.end()) ? it->second : 0.0)
               : ((it != ask_book_.end()) ? it->second : 0.0);
}

/**
 * @brief Returns the total quantity at the given price level on one side of the
 * book.
 *
 * Levels are 0-based:
 *   - For bids: level 0 = highest bid price, level 1 = second-highest, etc.
 *   - For asks: level 0 = lowest ask price, level 1 = second-lowest, etc.
 *
 * If `level` is negative or exceeds the number of price levels, this returns 0.
 *
 * @param side  Which side of the book to query (BookSide::Bid or BookSide::Ask)
 * @param level 0-based index of the price level
 * @return The quantity at that level, or 0 if out of range
 */
Quantity OrderBook::depth_at_level(const BookSide side, const int level) const {
    if (level < 0) return 0.0;

    // Choose which side of the book
    if (side == BookSide::Bid) {
        auto it = bid_book_.begin();
        auto end = bid_book_.end();
        for (int i = 0; it != end && i < level; ++it, ++i) {
        }
        return (it != end) ? it->second : 0.0;
    } else {
        auto it = ask_book_.begin();
        auto end = ask_book_.end();
        for (int i = 0; it != end && i < level; ++it, ++i) {
        }
        return (it != end) ? it->second : 0.0;
    }
}

/**
 * @brief Returns the price at the given price level on one side of the book.
 *
 * Levels are 0-based:
 *   - For bids: level 0 = highest bid price, level 1 = second-highest, etc.
 *   - For asks: level 0 = lowest ask price, level 1 = second-lowest, etc.
 *
 * If `level` is negative or exceeds the number of price levels, this returns 0.
 *
 * @param side  Which side of the book to query (BookSide::Bid or BookSide::Ask)
 * @param level 0-based index of the price level
 * @return The price at that level, or 0.0 if it does not exist
 */
Ticks OrderBook::price_at_level(const BookSide side,
                                const int level) const { // NEEDS TESTS
    if (level < 0) return 0.0;

    // Choose which side of the book
    if (side == BookSide::Bid) {
        auto it = bid_book_.begin();
        auto end = bid_book_.end();
        for (int i = 0; it != end && i < level; ++it, ++i) {
        }
        return (it != end) ? it->first : 0.0;
    } else {
        auto it = ask_book_.begin();
        auto end = ask_book_.end();
        for (int i = 0; it != end && i < level; ++it, ++i) {
        }
        return (it != end) ? it->first : 0.0;
    }
}

/**
 * @brief Returns the number of price levels currently in the bid book.
 *
 * @return Number of unique bid price levels.
 */
int OrderBook::bid_levels() const { return static_cast<int>(bid_book_.size()); }

/**
 * @brief Returns the number of price levels currently in the ask book.
 *
 * @return Number of unique ask price levels.
 */
int OrderBook::ask_levels() const { return static_cast<int>(ask_book_.size()); }

/**
 * @brief Returns if the orderbook is empty or not.
 *
 * @return true if empty, false if non-empty.
 */
bool OrderBook::is_empty() const {
    return bid_book_.empty() && ask_book_.empty();
}

/**
 * @brief Prints the top N levels of the order book to the console.
 *
 * This function displays the best `depth` number of price levels on both
 * the ask and bid sides. Output is printed in human-readable format with
 * price and corresponding quantity.
 *
 * @param depth The number of price levels to display from the top of each side.
 */
void OrderBook::print_top_levels(int depth) const {
    std::cout << "\n--- Order Book Top " << depth << " Levels ---\n";
    std::cout << " Ask:\n";
    int count = 0;
    for (const auto &[price, qty] : ask_book_) {
        std::cout << "  " << price << " @ " << qty << "\n";
        if (++count >= depth) break;
    }
    std::cout << " Bid:\n";
    count = 0;
    for (const auto &[price, qty] : bid_book_) {
        std::cout << "  " << price << " @ " << qty << "\n";
        if (++count >= depth) break;
    }
    std::cout << "--------------------------------\n";
}