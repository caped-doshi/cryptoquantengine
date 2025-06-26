/*
 * File: hftengine/core/data/readers/market_data_feed.cpp
 * Description: Class to stream trade and l2 data chronologically.
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
# include "../../market_data/l2_update.h"
# include "../../types/usings.h"
# include "../../orderbook/orderbook.h"
# include "../../types/event_type.h"
# include "l2_stream_reader.hpp"
# include "trade_stream_reader.hpp"


MarketDataFeed::MarketDataFeed(L2StreamReader& l2_stream, TradeStreamReader& trade_stream)
    : l2_stream_(l2_stream), trade_stream_(trade_stream) {}

bool MarketDataFeed::next_event(EventType& event_type, L2Update& l2_update, Trade& trade) {
    if (last_event_ == EventType::Trade) {
        trade_ok = trade_stream_.parse_next(trade);
    }
    else if (last_event_ == EventType::L2Update) {
        l2_ok = l2_stream_.parse_next(l2_update);
    }
    else {
        trade_ok = trade_stream_.parse_next(trade);
        l2_ok = l2_stream_.parse_next(l2_update);
    }
    if (trade_ok && l2_ok) {
        (trade.timestamp_ < l2_update.timestamp_)
            ? event_type = EventType::Trade
            : event_type = EventType::L2Update;
        last_event_ = event_type;
        return true;
    }
    else if (trade_ok) {
        event_type = EventType::Trade;
        last_event_ = event_type;
        return true;
    }
    else if (l2_ok) {
        event_type = EventType::L2Update;
        last_event_ = event_type;
        return true;
    }
    return false;
}