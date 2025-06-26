/*
 * File: tests/test_l2_stream_reader.cpp
 * Description: Unit tests for hftengine/core/data/readers/l2_stream_reader.cpp.
 * Author: Arvind Rathnashyam
 * Date: 2025-06-25
 * License: Proprietary
 */

# include <catch2/catch_test_macros.hpp>
# include <filesystem>
# include <fstream>

# include "core/data/readers/l2_stream_reader.hpp"
# include "core/types/book_side.h"
# include "core/types/update_type.h"
# include "core/market_data/l2_update.h"

TEST_CASE("L2StreamReader - Construction", "[l2][reader]") {
    SECTION("Valid tick and lot sizes") {
        REQUIRE_NOTHROW(L2StreamReader(0.01, 1.0));
    }
    
    SECTION("Invalid tick size") {
        REQUIRE_THROWS_AS(L2StreamReader(0.0, 1.0), std::invalid_argument);
    }
    
    SECTION("Invalid lot size") {
        REQUIRE_THROWS_AS(L2StreamReader(0.01, 0.0), std::invalid_argument);
    }
}

TEST_CASE("L2StreamReader - CSV Parsing", "[l2][csv]") {
    const std::string test_file = "test_l2_data.csv";
    {
        std::ofstream out(test_file);
        out << "timestamp,is_snapshot,side,price,amount\n";
        out << "123456789,true,bid,100.50,200.0\n";
        out << "123456790,false,ask,101.00,150.0\n";
    }

    L2StreamReader reader(0.01, 1.0);
    reader.open(test_file);

    SECTION("Parses valid bid snapshot correctly") {
        L2Update update;

        REQUIRE(reader.parse_next(update));
        REQUIRE(update.timestamp_ == 123456789);
        REQUIRE(update.update_type_ == UpdateType::Snapshot);
        REQUIRE(update.side_ == BookSide::Bid);
        REQUIRE(update.price_ == 100.5);
        REQUIRE(update.quantity_ == 200.0);
    }

    SECTION("Parses valid bid update correctly") {
        L2Update update;
        reader.parse_next(update); // each section gets fresh start

        REQUIRE(reader.parse_next(update));
        REQUIRE(update.timestamp_ == 123456790);
        REQUIRE(update.update_type_ == UpdateType::Incremental);
        REQUIRE(update.side_ == BookSide::Ask);
        REQUIRE(update.price_ == 101.0);
        REQUIRE(update.quantity_ == 150.0);
    }

    SECTION("Handles end of file") {
        L2Update update;
        reader.parse_next(update);  // Row 1
        reader.parse_next(update);  // Row 2
        REQUIRE_FALSE(reader.parse_next(update));  // No more data
    }

    // Clean up
    std::remove(test_file.c_str());
}