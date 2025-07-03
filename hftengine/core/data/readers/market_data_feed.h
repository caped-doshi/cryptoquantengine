/*
 * File: hftengine/core/data/readers/market_data_feed.h
 * Description: Class to stream trade and book_update data chronologically.
 * Author: Arvind Rathnashyam
 * Date: 2025-06-25
 * License: Proprietary
 */

#pragma once

#include <memory>
#include <optional>
#include <string>
#include <unordered_map>
#include <vector>
#include <cstdint>

#include "../../market_data/book_update.h"
#include "../../market_data/trade.h"
#include "../../orderbook/orderbook.h"
#include "../../types/event_type.h"
#include "../../types/usings.h"
#include "book_stream_reader.h"
#include "trade_stream_reader.h"

class MarketDataFeed {
  public:
    MarketDataFeed(const std::unordered_map<int, std::string> &book_files,
                   const std::unordered_map<int, std::string> &trade_files);
    bool next_event(int &asset_id, EventType &event_type,
                    BookUpdate &book_update, Trade &trade);
    std::optional<Timestamp> peek_timestamp();

  private:
    struct StreamState {
        std::unique_ptr<BookStreamReader> book_reader;
        std::unique_ptr<TradeStreamReader> trade_reader;

        std::optional<BookUpdate> next_book_update;
        std::optional<Trade> next_trade;

        bool advance_book();
        bool advance_trade();
    };
    std::map<int, StreamState> asset_streams_;
};