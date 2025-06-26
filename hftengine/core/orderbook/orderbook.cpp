/*
 * File: hftengine/core/orderbook/orderbook.cpp
 * Description:
 * Author: Arvind Rathnashyam
 * Date: 2025-06-24
 * License: Proprietary
 */

# include <algorithm>
# include <cmath>
# include <iostream>

# include "../types/book_side.h"
# include "../market_data/l2_update.h"
# include "../market_data/trade.h"
# include "orderbook.h"

OrderBook::OrderBook() : last_update_(UpdateType::Snapshot) {}

void OrderBook::clear() {
	bid_book_.clear();
	ask_book_.clear();
}

void OrderBook::apply_l2_update(const L2Update& update) {

	// throw exception on bad updates
	if (update.price_ <= 0.0) {
		throw std::invalid_argument("Price must be positive: " + std::to_string(update.price_));
	}

	if (update.quantity_ < 0.0) {
		throw std::invalid_argument("Quantity cannot be negative: " + std::to_string(update.quantity_));
	}

	if (update.update_type_ == UpdateType::Snapshot
		&& last_update_ == UpdateType::Incremental) {
		clear();
	}

	if (update.quantity_ == 0.0) {
		(update.side_ == BookSide::Bid) 
			? bid_book_.erase(update.price_) 
			: ask_book_.erase(update.price_);
	}
	else {
		(update.side_ == BookSide::Bid)
			? bid_book_[update.price_] = update.quantity_
			: ask_book_[update.price_] = update.quantity_;
	}
	last_update_ = update.update_type_;
}

Price OrderBook::best_bid() const {
	return bid_book_.empty() ? 0.0 : bid_book_.begin()->first;
}

Price OrderBook::best_ask() const {
	return ask_book_.empty() ? 0.0 : ask_book_.begin()->first;
}

Price OrderBook::mid_price() const {
	if (bid_book_.empty() || ask_book_.empty()) return 0.0;
	return (best_bid() + best_ask()) / 2.0;
}

Quantity OrderBook::depth_at(BookSide side, double price) const {
	auto it = (side == BookSide::Bid)
		? bid_book_.find(price)
		: ask_book_.find(price);
	return (side == BookSide::Bid) 
		? ((it != bid_book_.end()) ? it->second : 0.0) 
		: ((it != ask_book_.end()) ? it->second : 0.0);
}


bool OrderBook::is_empty() const {
	return bid_book_.empty() && ask_book_.empty();
}

void OrderBook::print_top_levels(int depth) const {
	std::cout << "\n--- Order Book Top " << depth << " Levels ---\n";

	std::cout << " Ask:\n";
	int count = 0;
	for (const auto& [price, qty] : ask_book_) {
		std::cout << "  " << price << " @ " << qty << "\n";
		if (++count >= depth) break;
	}

	std::cout << " Bid:\n";
	count = 0;
	for (const auto& [price, qty] : bid_book_) {
		std::cout << "  " << price << " @ " << qty << "\n";
		if (++count >= depth) break;
	}

	std::cout << "-----------------------------------\n";
}