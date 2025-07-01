/*
 * File: hftengine/core/data/readers/market_data_feed.h
 * Description: Class to stream trade and book_update data chronologically.
 * Author: Arvind Rathnashyam
 * Date: 2025-06-25
 * License: Proprietary
 */

#pragma once

# include <string>
# include <vector>
# include <unordered_map>
# include <memory>

# include "../../market_data/trade.h"
# include "../../market_data/book_update.h"
# include "../../types/usings.h"
# include "../../orderbook/orderbook.h"
# include "../../types/event_type.h"
# include "book_stream_reader.hpp"
# include "trade_stream_reader.hpp"


class MarketDataFeed {
public:
    MarketDataFeed(BookStreamReader& book_stream, TradeStreamReader& trade_stream);
	bool next_event(EventType& event_type, BookUpdate& book_update, Trade& trade);

private:
	BookStreamReader& book_stream_;
	TradeStreamReader& trade_stream_;

	bool trade_ok;
	bool book_ok;

	EventType last_event_;
};