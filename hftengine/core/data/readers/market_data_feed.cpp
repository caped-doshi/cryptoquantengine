/*
 * File: hftengine/core/data/readers/market_data_feed.cpp
 * Description: Class to stream trade and book_update data chronologically.
 * Author: Arvind Rathnashyam
 * Date: 2025-06-26
 * License: Proprietary
 */

#include <memory>
#include <string>
#include <unordered_map>
#include <vector>

#include "../../market_data/book_update.h"
#include "../../market_data/trade.h"
#include "../../orderbook/orderbook.h"
#include "../../types/event_type.h"
#include "../../types/usings.h"
#include "book_stream_reader.h"
#include "market_data_feed.h"
#include "trade_stream_reader.h"

MarketDataFeed::MarketDataFeed(
    const std::unordered_map<int, std::string> &book_files,
    const std::unordered_map<int, std::string> &trade_files) {
    for (const auto &[asset_id, book_file] : book_files) {
        StreamState state;
        state.book_reader = std::make_unique<BookStreamReader>();
        state.book_reader->open(book_file);

        auto trade_it = trade_files.find(asset_id);
        if (trade_it != trade_files.end()) {
            state.trade_reader = std::make_unique<TradeStreamReader>();
            state.trade_reader->open(trade_it->second);
        }

        asset_streams_[asset_id] = std::move(state);
    }
}

bool MarketDataFeed::next_event(int &asset_id, EventType &event_type,
                                BookUpdate &book_update, Trade &trade) {
    bool found = false;
    Timestamp min_time = std::numeric_limits<Timestamp>::max();

    for (auto &[id, stream] : asset_streams_) {
        // Ensure both streams are preloaded
        if (!stream.next_book_update.has_value()) stream.advance_book();
        if (!stream.next_trade.has_value()) stream.advance_trade();

        if (stream.next_book_update.has_value() &&
            stream.next_book_update->timestamp_ < min_time) {
            min_time = stream.next_book_update->timestamp_;
            asset_id = id;
            event_type = EventType::BookUpdate;
            found = true;
        }

        if (stream.next_trade.has_value() &&
            stream.next_trade->timestamp_ < min_time) {
            min_time = stream.next_trade->timestamp_;
            asset_id = id;
            event_type = EventType::Trade;
            found = true;
        }
    }

    if (!found) return false;

    auto &stream = asset_streams_.at(asset_id);
    if (event_type == EventType::BookUpdate) {
        book_update = *stream.next_book_update;
        stream.advance_book();
    } else {
        trade = *stream.next_trade;
        stream.advance_trade();
    }

    return true;
}

bool MarketDataFeed::StreamState::advance_book() {
    BookUpdate update;
    if (book_reader->parse_next(update)) {
        next_book_update = update;
        return true;
    }
    next_book_update.reset();
    return false;
}

bool MarketDataFeed::StreamState::advance_trade() {
    Trade t;
    if (trade_reader->parse_next(t)) {
        next_trade = t;
        return true;
    }
    next_trade.reset();
    return false;
}