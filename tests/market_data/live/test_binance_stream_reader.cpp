/*
 * Copyright (c) 2025 arvindkrv@protonmail.com
 *
 * Please see the LICENSE file for the terms and conditions
 * associated with this software.
 */

#include <catch2/catch_test_macros.hpp>
#include <string>

#include "core/market_data/book_update.h"
#include "core/market_data/readers/ws/binance_stream_reader.h"
#include "core/market_data/trade.h"
#include "core/types/enums/book_side.h"
#include "core/types/enums/trade_side.h"
#include "core/types/enums/update_type.h"

using namespace core::market_data;

TEST_CASE("[BinanceStreamReader] - live listen for 5 seconds",
          "[binance-ws][live]") {
    const std::string symbol = "xrpusdc";
    const std::string ws_uri =
        "wss://fstream.binance.com/stream?streams=" + symbol + "@depth@0ms/" +
        symbol + "@trade";
    const std::string rest_uri =
        "https://fapi.binance.com/fapi/v1/depth?symbol=" + symbol +
        "&limit=1000";
    const std::string book_csv = "test_live_book.csv";
    const std::string trade_csv = "test_live_trade.csv";
    const bool enable_csv_writer = false;

    BinanceStreamReader reader(ws_uri, rest_uri, book_csv, trade_csv,
                               enable_csv_writer);

    std::vector<BookUpdate> book_updates;
    std::vector<Trade> trades;

    auto start = std::chrono::steady_clock::now();
    auto end = start + std::chrono::seconds(5);

    while (std::chrono::steady_clock::now() < end) {
        BookUpdate bu;
        while (reader.parse_next_book(bu)) {
            book_updates.push_back(bu);
        }
        Trade tr;
        while (reader.parse_next_trade(tr)) {
            trades.push_back(tr);
        }
        std::this_thread::sleep_for(std::chrono::milliseconds(50));
    }

    REQUIRE(book_updates.size() > 0);
    INFO("Received " << book_updates.size() << " book updates and "
                     << trades.size() << " trades in 5 seconds.");
    
    //std::remove(book_csv.c_str());
    //std::remove(trade_csv.c_str());
}