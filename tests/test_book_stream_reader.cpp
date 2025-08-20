/*
 * File: tests/test_book_stream_reader.cpp
 * Description: Unit tests for hftengine/core/market_data/readers/book_stream_reader.cpp.
 * Author: Arvind Rathnashyam
 * Date: 2025-06-25
 * License: Proprietary
 */

# include <catch2/catch_test_macros.hpp>
# include <filesystem>
# include <fstream>

# include "core/market_data/readers/book_stream_reader.h"
# include "core/types/enums/book_side.h"
# include "core/types/enums/update_type.h"
# include "core/market_data/book_update.h"

TEST_CASE("[BookStreamReader] - CSV Parsing", "[book][csv]") {
    using namespace core::market_data;
    const std::string test_file = "test_book_update_data.csv";
    {
        std::ofstream out(test_file);
        out << "timestamp,local_timestamp,is_snapshot,side,price,amount\n";
        out << "123456789,123456791,true,bid,100.50,200.0\n";
        out << "123456790,123456792,false,ask,101.00,150.0\n";
    }

    BookStreamReader reader;
    reader.open(test_file);

    SECTION("Parses valid bid snapshot correctly") {
        BookUpdate update;

        REQUIRE(reader.parse_next(update));
        REQUIRE(update.exch_timestamp_ == 123456789);
        REQUIRE(update.local_timestamp_ == 123456791);
        REQUIRE(update.update_type_ == UpdateType::Snapshot);
        REQUIRE(update.side_ == BookSide::Bid);
        REQUIRE(update.price_ == 100.5);
        REQUIRE(update.quantity_ == 200.0);
    }

    SECTION("Parses valid bid update correctly") {
        BookUpdate update;
        reader.parse_next(update); // each section gets fresh start

        REQUIRE(reader.parse_next(update));
        REQUIRE(update.exch_timestamp_ == 123456790);
        REQUIRE(update.local_timestamp_ == 123456792);
        REQUIRE(update.update_type_ == UpdateType::Incremental);
        REQUIRE(update.side_ == BookSide::Ask);
        REQUIRE(update.price_ == 101.0);
        REQUIRE(update.quantity_ == 150.0);
    }

    SECTION("Handles end of file") {
        BookUpdate update;
        reader.parse_next(update);  // Row 1
        reader.parse_next(update);  // Row 2
        REQUIRE_FALSE(reader.parse_next(update));  // No more data
    }

    // Clean up
    std::remove(test_file.c_str());
}