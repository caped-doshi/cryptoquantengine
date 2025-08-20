/*
 * File: hftengine/core/market_data/market_data_feed.h
 * Description: Class to stream trade and book_update data chronologically.
 * Author: Arvind Rathnashyam
 * Date: 2025-06-25
 * License: Proprietary
 */

#pragma once

#include <cstdint>
#include <memory>
#include <optional>
#include <string>
#include <unordered_map>
#include <vector>

#include "../orderbook/orderbook.h"
#include "../types/enums/event_type.h"
#include "../types/aliases/usings.h"
#include "book_update.h"
#include "readers/book_stream_reader.h"
#include "readers/trade_stream_reader.h"
#include "trade.h"

namespace core::market_data {
class MarketDataFeed {
  public:
    MarketDataFeed();
    MarketDataFeed(const std::unordered_map<int, std::string> &book_files,
                   const std::unordered_map<int, std::string> &trade_files);

    void add_stream(int asset_id, const std::string &book_file,
                    const std::string &trade_file);
    bool next_event(int &asset_id, EventType &event_type,
                    core::market_data::BookUpdate &book_update,
                    core::market_data::Trade &trade);
    std::optional<Timestamp> peek_timestamp();

  private:
    struct StreamState {
        std::unique_ptr<core::market_data::BookStreamReader> book_reader;
        std::unique_ptr<core::market_data::TradeStreamReader> trade_reader;

        std::optional<core::market_data::BookUpdate> next_book_update;
        std::optional<core::market_data::Trade> next_trade;

        bool advance_book();
        bool advance_trade();
    };
    std::map<int, StreamState> asset_streams_;
};
} // namespace core::market_data