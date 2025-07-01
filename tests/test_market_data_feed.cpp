/*
 * File: tests/test_market_data_feed.cpp
 * Description: Unit tests for hftengine/core/data/readers/market_data_feed.cpp.
 * Author: Arvind Rathnashyam
 * Date: 2025-06-25
 * License: Proprietary
 */

# include <catch2/catch_test_macros.hpp>
# include <filesystem>
# include <fstream>

# include "core/data/readers/market_data_feed.hpp"
# include "core/data/readers/book_stream_reader.hpp"
# include "core/data/readers/trade_stream_reader.hpp"
# include "core/market_data/book_update.h"
# include "core/market_data/trade.h"

 // Helper: Create temporary CSV files for testing
namespace TestHelpers {
    void create_trade_csv(const std::string& filename) {
        std::ofstream f(filename);
        f << "timestamp,local_timestamp,id,side,price,amount\n"
            << "100,110,1,buy,50000.0,1.0\n"   // Trade 1
            << "300,310,2,sell,50001.0,0.5\n"; // Trade 2
    }

    void create_book_update_csv(const std::string& filename) {
        std::ofstream f(filename);
        f << "timestamp,is_snapshot,side,price,amount\n"
            << "200,false,bid,50000.0,2.0\n"   // Book Update 1
            << "400,false,ask,50001.0,1.5\n"  // Book Update 2
            << "500,false,ask,50001.0,2.5\n";  // Book Update 3
    }
}

TEST_CASE("[MarketDataFeed] - initializes with empty state", "[feed][empty]") {
    BookStreamReader book_reader(1.0, 1.0);
    TradeStreamReader trade_reader(1.0, 1.0);
    MarketDataFeed feed(book_reader, trade_reader);

    EventType type;
    BookUpdate book_update;
    Trade trade;

    REQUIRE_FALSE(feed.next_event(type, book_update, trade));  // No data loaded yet
}

TEST_CASE("[MarketDataFeed] - processes events in timestamp order", "[feed][order]") {
    // Setup test files
    const std::string trade_file = "test_trades.csv";
    const std::string book_update_file = "test_book_update.csv";
    TestHelpers::create_trade_csv(trade_file);
    TestHelpers::create_book_update_csv(book_update_file);

    // Initialize readers and feed
    BookStreamReader book_reader(1.0, 1.0);
    TradeStreamReader trade_reader(1.0, 1.0);
    book_reader.open(book_update_file);
    trade_reader.open(trade_file);
    MarketDataFeed feed(book_reader, trade_reader);

    // Expected sequence: Trade@100, L2@200, Trade@300, L2@400
    SECTION("Correct event order") {
        EventType type;
        BookUpdate book_update;
        Trade trade;

        REQUIRE(feed.next_event(type, book_update, trade));
        REQUIRE(type == EventType::Trade);
        REQUIRE(trade.timestamp_ == 100);

        REQUIRE(feed.next_event(type, book_update, trade));
        REQUIRE(type == EventType::BookUpdate);
        REQUIRE(book_update.timestamp_ == 200);

        REQUIRE(feed.next_event(type, book_update, trade));
        REQUIRE(type == EventType::Trade);
        REQUIRE(trade.timestamp_ == 300);

        REQUIRE(feed.next_event(type, book_update, trade));
        REQUIRE(type == EventType::BookUpdate);
        REQUIRE(book_update.timestamp_ == 400);

        REQUIRE(feed.next_event(type, book_update, trade));
        REQUIRE(type == EventType::BookUpdate);
        REQUIRE(book_update.timestamp_ == 500);

        REQUIRE_FALSE(feed.next_event(type, book_update, trade));  // EOF
    }

    // Cleanup
    std::filesystem::remove(trade_file);
    std::filesystem::remove(book_update_file);
}

TEST_CASE("[MarketDataFeed] - handles files with headers only") {
    const std::string header_only_file = "header_only.csv";

    // Create file with headers but no data
    {
        std::ofstream f(header_only_file);
        f << "timestamp,is_snapshot,side,price,amount\n";  // L2 header only
    }

    BookStreamReader book_reader(1.0, 1.0);
    TradeStreamReader trade_reader(1.0, 1.0);

    SECTION("MarketDataFeed handles header-only file") {
        book_reader.open(header_only_file);

        // Create similar header-only trade file
        const std::string trade_header_file = "trade_header.csv";
        {
            std::ofstream f(trade_header_file);
            f << "timestamp,local_timestamp,id,side,price,amount\n";
        }
        trade_reader.open(trade_header_file);

        MarketDataFeed feed(book_reader, trade_reader);
        EventType type;
        BookUpdate book_update;
        Trade trade;

        REQUIRE_FALSE(feed.next_event(type, book_update, trade)); // No events

        std::filesystem::remove(trade_header_file);
    }

    std::filesystem::remove(header_only_file);
}