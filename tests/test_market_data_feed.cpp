/*
 * File: tests/test_market_data_feed.cpp
 * Description: Unit tests for hftengine/core/data/readers/market_data_feed.cpp.
 * Author: Arvind Rathnashyam
 * Date: 2025-06-25
 * License: Proprietary
 */

#include <catch2/catch_test_macros.hpp>
#include <filesystem>
#include <fstream>
#include <set>
#include <tuple>
#include <unordered_map>
#include <vector>
#include <cstdint>

#include "core/data/readers/book_stream_reader.h"
#include "core/data/readers/market_data_feed.h"
#include "core/data/readers/trade_stream_reader.h"
#include "core/market_data/book_update.h"
#include "core/market_data/trade.h"

// Helper: Create temporary CSV files for testing
namespace TestHelpers {
void create_trade_csv(const std::string &filename) {
    std::ofstream f(filename);
    f << "timestamp,local_timestamp,id,side,price,amount\n"
      << "100,110,1,buy,50000.0,1.0\n"
      << "300,310,2,sell,50001.0,0.5\n";
}

void create_book_update_csv(const std::string &filename) {
    std::ofstream f(filename);
    f << "timestamp,local_timestamp,is_snapshot,side,price,amount\n"
      << "200,210,false,bid,50000.0,2.0\n"
      << "400,410,false,ask,50001.0,1.5\n"
      << "500,510,false,ask,50001.0,2.5\n";
}
void create_trade_csv_2(const std::string &filename) {
    std::ofstream f(filename);
    f << "timestamp,local_timestamp,id,side,price,amount\n"
      << "150,160,1,buy,2000.0,1.0\n"
      << "330,340,2,sell,2001.0,0.5\n";
}

void create_book_update_csv_2(const std::string &filename) {
    std::ofstream f(filename);
    f << "timestamp,local_timestamp,is_snapshot,side,price,amount\n"
      << "220,230,false,bid,2000.0,2.0\n"
      << "430,440,false,ask,2001.0,1.5\n"
      << "540,550,false,ask,2001.0,2.5\n";
}
} // namespace TestHelpers

TEST_CASE("[MarketDataFeed] - initializes with empty input",
          "[multi-asset][empty]") {
    MarketDataFeed feed({}, {});
    EventType type;
    BookUpdate book_update;
    Trade trade;
    int asset_id;

    REQUIRE_FALSE(feed.next_event(asset_id, type, book_update, trade));
}

TEST_CASE("[MarketDataFeed] - processes multi-asset events in timestamp order",
          "[multi-asset][order]") {
    const std::string book_file = "book_asset0.csv";
    const std::string trade_file = "trade_asset0.csv";
    TestHelpers::create_book_update_csv(book_file);
    TestHelpers::create_trade_csv(trade_file);

    std::unordered_map<int, std::string> book_files = {{0, book_file}};
    std::unordered_map<int, std::string> trade_files = {{0, trade_file}};

    MarketDataFeed feed(book_files, trade_files);

    EventType type;
    BookUpdate book_update;
    Trade trade;
    int asset_id;

    SECTION("Correct order of events for asset 0") {
        REQUIRE(feed.next_event(asset_id, type, book_update, trade));
        REQUIRE(asset_id == 0);
        REQUIRE(type == EventType::Trade);
        REQUIRE(trade.timestamp_ == 100);

        REQUIRE(feed.next_event(asset_id, type, book_update, trade));
        REQUIRE(asset_id == 0);
        REQUIRE(type == EventType::BookUpdate);
        REQUIRE(book_update.timestamp_ == 200);

        REQUIRE(feed.next_event(asset_id, type, book_update, trade));
        REQUIRE(asset_id == 0);
        REQUIRE(type == EventType::Trade);
        REQUIRE(trade.timestamp_ == 300);

        REQUIRE(feed.next_event(asset_id, type, book_update, trade));
        REQUIRE(asset_id == 0);
        REQUIRE(type == EventType::BookUpdate);
        REQUIRE(book_update.timestamp_ == 400);

        REQUIRE(feed.next_event(asset_id, type, book_update, trade));
        REQUIRE(asset_id == 0);
        REQUIRE(type == EventType::BookUpdate);
        REQUIRE(book_update.timestamp_ == 500);

        REQUIRE_FALSE(feed.next_event(asset_id, type, book_update, trade));
    }

    std::filesystem::remove(book_file);
    std::filesystem::remove(trade_file);
}

TEST_CASE("[MarketDataFeed] - handles header-only files",
          "[multi-asset][header-only]") {
    const std::string book_file = "book_empty.csv";
    const std::string trade_file = "trade_empty.csv";

    {
        std::ofstream f(book_file);
        f << "timestamp,local_timestamp,is_snapshot,side,price,amount\n";
    }
    {
        std::ofstream f(trade_file);
        f << "timestamp,local_timestamp,id,side,price,amount\n";
    }

    MarketDataFeed feed({{0, book_file}}, {{0, trade_file}});

    EventType type;
    BookUpdate book_update;
    Trade trade;
    int asset_id;

    REQUIRE_FALSE(feed.next_event(asset_id, type, book_update, trade));

    std::filesystem::remove(book_file);
    std::filesystem::remove(trade_file);
}

TEST_CASE("[MarketDataFeed] - multi-asset event sequencing",
          "[feed][multi_asset]") {
    // Create test files for 2 assets
    const std::string btc_trade_file = "btc_trades.csv";
    const std::string btc_book_file = "btc_book.csv";
    const std::string eth_trade_file = "eth_trades.csv";
    const std::string eth_book_file = "eth_book.csv";

    TestHelpers::create_trade_csv(btc_trade_file);
    TestHelpers::create_book_update_csv(btc_book_file);
    TestHelpers::create_trade_csv_2(eth_trade_file);
    TestHelpers::create_book_update_csv_2(eth_book_file);

    // Prepare file maps
    std::unordered_map<int, std::string> trade_files = {{0, btc_trade_file},
                                                        {1, eth_trade_file}};
    std::unordered_map<int, std::string> book_files = {{0, btc_book_file},
                                                       {1, eth_book_file}};

    MarketDataFeed feed(book_files, trade_files);

    std::vector<std::tuple<int, EventType, Timestamp>> observed_events;
    BookUpdate book_update;
    Trade trade;
    EventType event_type;
    int asset_id;

    // Collect all events
    while (feed.next_event(asset_id, event_type, book_update, trade)) {
        Timestamp ts;
        (event_type == EventType::Trade) ? ts = trade.timestamp_
                                         : ts = book_update.timestamp_;
        observed_events.emplace_back(asset_id, event_type, ts);
    }
    REQUIRE(std::get<1>(observed_events[0]) == EventType::Trade);
    REQUIRE(std::get<2>(observed_events[0]) == 100);
    REQUIRE(std::get<1>(observed_events[1]) == EventType::Trade);
    REQUIRE(std::get<2>(observed_events[1]) == 150);
    REQUIRE(std::get<1>(observed_events[2]) == EventType::BookUpdate);
    REQUIRE(std::get<1>(observed_events[3]) == EventType::BookUpdate);

    // Check global timestamp ordering
    for (size_t i = 1; i < observed_events.size(); ++i) {
        REQUIRE(std::get<2>(observed_events[i]) >=
                std::get<2>(observed_events[i - 1]));
    }
    
    // Check that we have events from both assets
    std::set<int> assets;
    for (auto &[asset_id, type, ts] : observed_events) {
        assets.insert(asset_id);
    }
    REQUIRE(assets.count(0) == 1);
    REQUIRE(assets.count(1) == 1);

    // Cleanup
    std::filesystem::remove(btc_trade_file);
    std::filesystem::remove(btc_book_file);
    std::filesystem::remove(eth_trade_file);
    std::filesystem::remove(eth_book_file);
}