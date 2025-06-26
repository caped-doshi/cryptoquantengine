/*
 * File: hftengine/core/data/readers/market_data_feed.h
 * Description: Class to stream trade and l2 data chronologically.
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
# include "../../market_data/l2_update.h"
# include "../../types/usings.h"
# include "../../orderbook/orderbook.h"
# include "../../types/event_type.h"
# include "l2_stream_reader.hpp"
# include "trade_stream_reader.hpp"


class MarketDataFeed {
public:
    MarketDataFeed(L2StreamReader& l2_stream, TradeStreamReader& trade_stream);
	bool next_event(EventType& event_type, L2Update& l2_update, Trade& trade);

private:
	L2StreamReader& l2_stream_;
	TradeStreamReader& trade_stream_;

	bool trade_ok;
	bool l2_ok;

	EventType last_event_;
};