/*
 * File: tests/test_market_data_feed.cpp
 * Description: Unit tests for hftengine/core/market_data/market_data_feed.cpp.
 * Author: Arvind Rathnashyam
 * Date: 2025-06-25
 * License: Proprietary
 */

#include <catch2/catch_test_macros.hpp>
#include <cstdint>
#include <filesystem>
#include <fstream>
#include <set>
#include <tuple>
#include <unordered_map>
#include <vector>

#include "core/market_data/book_update.h"
#include "core/market_data/market_data_feed.h"
#include "core/market_data/readers/book_stream_reader.h"
#include "core/market_data/readers/trade_stream_reader.h"
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
    using namespace core::market_data;

    MarketDataFeed feed({}, {});
    EventType type;
    BookUpdate book_update;
    Trade trade;
    int asset_id;

    REQUIRE_FALSE(feed.next_event(asset_id, type, book_update, trade));
}

TEST_CASE("[MarketDataFeed] - add_stream single asset",
          "[MarketDataFeed][add_stream]") {
    using namespace core::market_data;

    const std::string book_file = "test_book.csv";
    const std::string trade_file = "test_trade.csv";
    TestHelpers::create_book_update_csv(book_file);
    TestHelpers::create_trade_csv(trade_file);

    MarketDataFeed feed;
    feed.add_stream(1, book_file, trade_file);

    EventType event_type;
    BookUpdate book_update;
    Trade trade;
    int asset_id;

    REQUIRE(feed.next_event(asset_id, event_type, book_update, trade));
    REQUIRE(asset_id == 1);
    REQUIRE(event_type == EventType::Trade);
    REQUIRE(trade.exch_timestamp_ == 100);

    std::filesystem::remove(book_file);
    std::filesystem::remove(trade_file);
}

TEST_CASE("[MarketDataFeed] - processes multi-asset events in timestamp order",
          "[multi-asset][order]") {
    using namespace core::market_data;

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
        REQUIRE(trade.exch_timestamp_ == 100);

        REQUIRE(feed.next_event(asset_id, type, book_update, trade));
        REQUIRE(asset_id == 0);
        REQUIRE(type == EventType::BookUpdate);
        REQUIRE(book_update.exch_timestamp_ == 200);

        REQUIRE(feed.next_event(asset_id, type, book_update, trade));
        REQUIRE(asset_id == 0);
        REQUIRE(type == EventType::Trade);
        REQUIRE(trade.exch_timestamp_ == 300);

        REQUIRE(feed.next_event(asset_id, type, book_update, trade));
        REQUIRE(asset_id == 0);
        REQUIRE(type == EventType::BookUpdate);
        REQUIRE(book_update.exch_timestamp_ == 400);

        REQUIRE(feed.next_event(asset_id, type, book_update, trade));
        REQUIRE(asset_id == 0);
        REQUIRE(type == EventType::BookUpdate);
        REQUIRE(book_update.exch_timestamp_ == 500);

        REQUIRE_FALSE(feed.next_event(asset_id, type, book_update, trade));
    }

    std::filesystem::remove(book_file);
    std::filesystem::remove(trade_file);
}

TEST_CASE("[MarketDataFeed] - handles header-only files",
          "[multi-asset][header-only]") {
    using namespace core::market_data;

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
    using namespace core::market_data;
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
        (event_type == EventType::Trade) ? ts = trade.exch_timestamp_
                                         : ts = book_update.exch_timestamp_;
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

TEST_CASE("[MarketDataFeed] - peek timestamp", "[feed][peek]") {
    using namespace core::market_data;
    // Create test files for 2 assets
    const std::string trade_file_1 = "btc_trades.csv";
    const std::string book_file_1 = "btc_book.csv";
    const std::string trade_file_2 = "eth_trades.csv";
    const std::string book_file_2 = "eth_book.csv";

    TestHelpers::create_trade_csv(trade_file_1);
    TestHelpers::create_book_update_csv(book_file_1);
    TestHelpers::create_trade_csv_2(trade_file_2);
    TestHelpers::create_book_update_csv_2(book_file_2);

    std::unordered_map<int, std::string> trade_files = {{0, trade_file_1},
                                                        {1, trade_file_2}};
    std::unordered_map<int, std::string> book_files = {{0, book_file_1},
                                                       {1, book_file_2}};

    MarketDataFeed feed(book_files, trade_files);

    SECTION("initial peek behavior") {
        auto ts_opt = feed.peek_timestamp();
        REQUIRE(ts_opt.has_value());
        REQUIRE(ts_opt.value() == 100); // Earliest is trade at 100 for asset 0
    }
    SECTION("peek does not affect the next_event") {
        auto ts_opt = feed.peek_timestamp();
        // Advance one event (trade at 100)
        int asset_id;
        EventType type;
        BookUpdate book_update;
        Trade trade;
        REQUIRE(feed.next_event(asset_id, type, book_update, trade));
        // ensure the timestamp is not affected by the peak
        REQUIRE(type == EventType::Trade);
        REQUIRE(trade.exch_timestamp_ == 100);
    }
    SECTION("peak after next_event") {
        auto ts_opt = feed.peek_timestamp();
        int asset_id;
        EventType type;
        BookUpdate book_update;
        Trade trade;
        feed.next_event(asset_id, type, book_update, trade);
        // Now earliest should be trade at 150 for asset 1
        ts_opt = feed.peek_timestamp();
        REQUIRE(ts_opt.has_value());
        REQUIRE(ts_opt.value() == 150);
    }
    // Cleanup
    std::filesystem::remove(trade_file_1);
    std::filesystem::remove(book_file_1);
    std::filesystem::remove(trade_file_2);
    std::filesystem::remove(book_file_2);
}