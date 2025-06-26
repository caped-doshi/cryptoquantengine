# include <catch2/catch_test_macros.hpp>
# include <filesystem>
# include <fstream>

# include "core/data/readers/trade_stream_reader.hpp"
# include "core/types/trade_side.h"
# include "core/market_data/trade.h"


TEST_CASE("TradeStreamReader - Construction", "[trade][reader]") {
    SECTION("Valid tick and lot sizes") {
        REQUIRE_NOTHROW(TradeStreamReader(0.01, 1.0));
    }

    SECTION("Invalid tick size") {
        REQUIRE_THROWS_AS(TradeStreamReader(0.0, 1.0), std::invalid_argument);
    }

    SECTION("Invalid lot size") {
        REQUIRE_THROWS_AS(TradeStreamReader(0.01, 0.0), std::invalid_argument);
    }
}

TEST_CASE("TradeStreamReader - CSV Parsing", "[trade][csv]") {
    const std::string test_file = "test_trade_data.csv";
    {
        std::ofstream out(test_file);
        out << "timestamp,local_timestamp,id,side,price,amount\n";
        out << "1740009604700000,1740009604703670,47311612,buy,2.7347,4.2\n";
        out << "1740009604840000,1740009604859720,47311613,sell,2.7346,76.8\n";
    }

    TradeStreamReader reader(0.0001, 0.1);
    reader.open(test_file);

    SECTION("Parses valid bid snapshot correctly") {
        Trade trade;

        REQUIRE(reader.parse_next(trade));
        REQUIRE(trade.timestamp_ == 1740009604700000);
        REQUIRE(trade.local_timestamp_ == 1740009604703670);
        REQUIRE(trade.orderId_ == 47311612);
        REQUIRE(trade.side_ == TradeSide::Buy);
        REQUIRE(trade.price_ == 2.7347);
        REQUIRE(trade.quantity_ == 4.2);
    }

    SECTION("Parses valid bid update correctly") {
        Trade trade;
        reader.parse_next(trade); // each section gets fresh start

        REQUIRE(reader.parse_next(trade));
        REQUIRE(trade.timestamp_ == 1740009604840000);
        REQUIRE(trade.local_timestamp_ == 1740009604859720);
        REQUIRE(trade.orderId_ == 47311613);
        REQUIRE(trade.side_ == TradeSide::Sell);
        REQUIRE(trade.price_ == 2.7346);
        REQUIRE(trade.quantity_ == 76.8);
    }

    SECTION("Handles end of file") {
        Trade trade;
        reader.parse_next(trade);  // Row 1
        reader.parse_next(trade);  // Row 2
        REQUIRE_FALSE(reader.parse_next(trade));  // No more data
    }

    // Clean up
    std::remove(test_file.c_str());
}