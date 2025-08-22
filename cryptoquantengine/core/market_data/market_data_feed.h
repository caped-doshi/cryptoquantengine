/*
 * Copyright (c) 2025 arvindkrv@protonmail.com
 *
 * Please see the LICENSE file for the terms and conditions
 * associated with this software.
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
    void set_market_feed_latency(Microseconds latency_us);

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
    Microseconds market_feed_latency_us_ = 10'000;
};
} // namespace core::market_data