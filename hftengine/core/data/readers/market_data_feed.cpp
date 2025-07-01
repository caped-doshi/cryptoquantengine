/*
 * File: hftengine/core/data/readers/market_data_feed.cpp
 * Description: Class to stream trade and book_update data chronologically.
 * Author: Arvind Rathnashyam
 * Date: 2025-06-26
 * License: Proprietary
 */

# include <string>
# include <vector>
# include <unordered_map>
# include <memory>

# include "market_data_feed.hpp"
# include "../../market_data/trade.h"
# include "../../market_data/book_update.h"
# include "../../types/usings.h"
# include "../../orderbook/orderbook.h"
# include "../../types/event_type.h"
# include "book_stream_reader.hpp"
# include "trade_stream_reader.hpp"


MarketDataFeed::MarketDataFeed(BookStreamReader& book_stream, TradeStreamReader& trade_stream)
    : book_stream_(book_stream), trade_stream_(trade_stream) {}

bool MarketDataFeed::next_event(EventType& event_type, BookUpdate& book_update, Trade& trade) {
    if (last_event_ == EventType::Trade) {
        trade_ok = trade_stream_.parse_next(trade);
    }
    else if (last_event_ == EventType::BookUpdate) {
        book_ok = book_stream_.parse_next(book_update);
    }
    else {
        trade_ok = trade_stream_.parse_next(trade);
        book_ok = book_stream_.parse_next(book_update);
    }
    if (trade_ok && book_ok) {
        (trade.timestamp_ < book_update.timestamp_)
            ? event_type = EventType::Trade
            : event_type = EventType::BookUpdate;
        last_event_ = event_type;
        return true;
    }
    else if (trade_ok) {
        event_type = EventType::Trade;
        last_event_ = event_type;
        return true;
    }
    else if (book_ok) {
        event_type = EventType::BookUpdate;
        last_event_ = event_type;
        return true;
    }
    return false;
}