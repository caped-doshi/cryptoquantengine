/*
 * Copyright (c) 2025 arvindkrv@protonmail.com
 *
 * Please see the LICENSE file for the terms and conditions
 * associated with this software.
 */

#include <algorithm>
#include <cmath>
#include <iomanip>
#include <iostream>

#include "../../utils/logger/logger.h"
#include "../../utils/math/math_utils.h"
#include "../market_data/book_update.h"
#include "../market_data/trade.h"
#include "../types/enums/book_side.h"
#include "orderbook.h"

namespace core::orderbook {

OrderBook::OrderBook(double tick_size, double lot_size,
                     std::shared_ptr<utils::logger::Logger> logger)
    : tick_size_(tick_size), lot_size_(lot_size), bid_book_(), ask_book_(),
      last_update_(UpdateType::Snapshot), logger_(logger) {
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
 * @brief Returns the bid book as a map of price levels to quantities.
 *
 * The bid book is sorted in descending order of price.
 *
 * @return A map of price levels (Ticks) to quantities (Quantity) for asks.
 */
std::unordered_map<Ticks, Quantity> OrderBook::bid_book() const {
    return bid_book_;
}

/**
 * @brief Returns the ask book as a map of price levels to quantities.
 *
 * The ask book is sorted in ascending order of price.
 *
 * @return A map of price levels (Ticks) to quantities (Quantity) for asks.
 */
std::unordered_map<Ticks, Quantity> OrderBook::ask_book() const {
    return ask_book_;
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
void OrderBook::apply_book_update(const core::market_data::BookUpdate &update) {

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
    Ticks price_ticks = utils::math::price_to_ticks(update.price_, tick_size_);
    if (update.quantity_ == 0.0) {
        (update.side_ == BookSide::Bid) ? bid_book_.erase(price_ticks)
                                        : ask_book_.erase(price_ticks);
    } else {
        (update.side_ == BookSide::Bid)
            ? bid_book_[price_ticks] = update.quantity_
            : ask_book_[price_ticks] = update.quantity_;
    }
    (update.side_ == BookSide::Bid) ? bids_cache_valid_ = false
                                    : asks_cache_valid_ = false;
    last_update_ = update.update_type_;
}

/**
 * @brief Returns the best (highest) bid price currently in the bid book.
 *
 * @return The best bid price, or 0.0 if the bid book is empty.
 */
Price OrderBook::best_bid() const {
    if (bid_book_.empty()) return 0.0;
    if (bids_cache_valid_)
        return utils::math::ticks_to_price(cached_sorted_bids_.front().first,
                                           tick_size_);
    auto it = std::max_element(
        bid_book_.begin(), bid_book_.end(),
        [](const auto &a, const auto &b) { return a.first < b.first; });
    return utils::math::ticks_to_price(it->first, tick_size_);
}

/**
 * @brief Returns the best (lowest) ask price currently in the ask book.
 *
 * @return The best ask price, or 0.0 if the ask book is empty.
 */
Price OrderBook::best_ask() const {
    if (ask_book_.empty()) return 0.0;
    if (asks_cache_valid_)
        return utils::math::ticks_to_price(cached_sorted_asks_.front().first,
                                           tick_size_);
    auto it = std::min_element(
        ask_book_.begin(), ask_book_.end(),
        [](const auto &a, const auto &b) { return a.first < b.first; });
    return utils::math::ticks_to_price(it->first, tick_size_);
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
Quantity OrderBook::depth_at(const BookSide side, Ticks price) const {
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
Quantity OrderBook::depth_at_level(const BookSide side, int level) const {
    if (level < 0) return 0.0;
    const auto &book = (side == BookSide::Bid) ? sorted_bids() : sorted_asks();
    if (level >= static_cast<int>(book.size())) return 0.0;
    return book[level].second;
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
Ticks OrderBook::price_at_level(const BookSide side, int level) const {
    if (level < 0) return 0.0;
    const auto &book = (side == BookSide::Bid) ? sorted_bids() : sorted_asks();
    if (level >= static_cast<int>(book.size())) return 0.0;
    return book[level].first;
}

/**
 * @brief Returns a vector of all bids sorted in descending order of price.
 *
 * @return A vector of pairs, where each pair contains a price level (Ticks)
 * and the corresponding quantity (Quantity) at that level.
 */
std::vector<std::pair<Ticks, Quantity>> OrderBook::sorted_bids() const {
    if (bids_cache_valid_) return cached_sorted_bids_;
    cached_sorted_bids_.assign(bid_book_.begin(), bid_book_.end());
    std::sort(cached_sorted_bids_.begin(), cached_sorted_bids_.end(),
              [](const auto &a, const auto &b) { return a.first > b.first; });
    bids_cache_valid_ = true;
    return cached_sorted_bids_;
}

/**
 * @brief Returns a vector of all asks sorted in ascending order of price.
 *
 * @return A vector of pairs, where each pair contains a price level (Ticks)
 * and the corresponding quantity (Quantity) at that level.
 */
std::vector<std::pair<Ticks, Quantity>> OrderBook::sorted_asks() const {
    if (asks_cache_valid_) return cached_sorted_asks_;
    cached_sorted_asks_.assign(ask_book_.begin(), ask_book_.end());
    std::sort(cached_sorted_asks_.begin(), cached_sorted_asks_.end(),
              [](const auto &a, const auto &b) { return a.first < b.first; });
    asks_cache_valid_ = true;
    return cached_sorted_asks_;
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
    std::ostringstream oss;
    oss << "[OrderBook] Top " << depth << " levels:\n";
    oss << "Bids:\n";
    int count = 0;
    for (const auto &[price, qty] : bid_book_) {
        if (count++ >= depth) break;
        oss << "  " << std::fixed << std::setprecision(8)
            << utils::math::ticks_to_price(price, tick_size_) << " : " << qty
            << "\n";
    }
    oss << "Asks:\n";
    count = 0;
    for (const auto &[price, qty] : ask_book_) {
        if (count++ >= depth) break;
        oss << "  " << std::fixed << std::setprecision(8)
            << utils::math::ticks_to_price(price, tick_size_) << " : " << qty
            << "\n";
    }
    if (logger_) {
        logger_->log(oss.str(), utils::logger::LogLevel::Info);
    } else {
        std::cout << oss.str();
    }
}
} // namespace core::orderbook