/*
 * File: tests/test_config_reader.cpp
 * Description: Unit tests for hftengine/core/orderbook/orderbook.cpp.
 * Author: Arvind Rathnashyam
 * Date: 2025-06-25
 * License: Proprietary
 */

#include <catch2/catch_test_macros.hpp>

# include "core/orderbook/orderbook.h"
# include "core/types/update_type.h"
# include "core/types/book_side.h"
# include "core/market_data/l2_update.h"

TEST_CASE("[OrderBook] - Initial State", "[orderbook][init]") {
    OrderBook book;
    
    REQUIRE(book.is_empty());
    REQUIRE(book.best_bid() == 0.0);
    REQUIRE(book.best_ask() == 0.0);
    REQUIRE(book.mid_price() == 0.0);
}

TEST_CASE("[OrderBook] - L2 Update Processing", "[orderbook][updates]") {
    OrderBook book;

    SECTION("Snapshot updates initialize book") {
        L2Update snapshot_bid{ 0, 90, UpdateType::Snapshot, BookSide::Bid, 100.0, 500.0 };
        book.apply_l2_update(snapshot_bid);

        REQUIRE_FALSE(book.is_empty());
        REQUIRE(book.best_bid() == 100.0);
        REQUIRE(book.best_ask() == 0.0);
        REQUIRE(book.depth_at(BookSide::Bid, 100.0) == 500.0);
    }

    SECTION("Incremental updates modify existing levels") {
        // Setup with snapshot
        book.apply_l2_update({ 10, 100, UpdateType::Snapshot, BookSide::Ask, 101.0, 200.0 });

        // Incremental update
        book.apply_l2_update({ 20, 110, UpdateType::Incremental, BookSide::Ask, 101.0, 150.0 });
        REQUIRE(book.depth_at(BookSide::Ask, 101.0) == 150.0);
    }

    SECTION("Zero quantity removes price level") {
        book.apply_l2_update({ 30, 120, UpdateType::Snapshot, BookSide::Bid, 99.0, 300.0 });
        book.apply_l2_update({ 40, 130, UpdateType::Incremental, BookSide::Bid, 99.0, 0.0 });

        REQUIRE(book.depth_at(BookSide::Bid, 99.0) == 0.0);
        REQUIRE(book.is_empty());
    }
}

TEST_CASE("[OrderBook] - Price Level Priority", "[orderbook][priority]") {
    OrderBook book;

    // snapshots
    SECTION("Bid book is sorted in descending order from snapshots") {
        book.apply_l2_update({ 0, 0, UpdateType::Snapshot, BookSide::Bid, 100.0, 100.0 });
        book.apply_l2_update({ 0, 0, UpdateType::Snapshot, BookSide::Bid, 99.0, 200.0 });
        book.apply_l2_update({ 0, 0, UpdateType::Snapshot, BookSide::Bid, 101.0, 300.0 });

        REQUIRE(book.best_bid() == 101.0);  // Highest bid first
    }

    SECTION("Ask book is sorted in ascending order from snapshots") {
        book.apply_l2_update({ 0, 0, UpdateType::Snapshot, BookSide::Ask, 101.0, 100.0 });
        book.apply_l2_update({ 0, 0, UpdateType::Snapshot, BookSide::Ask, 102.0, 200.0 });
        book.apply_l2_update({ 0, 0, UpdateType::Snapshot, BookSide::Ask, 100.0, 300.0 });

        REQUIRE(book.best_ask() == 100.0);  // Lowest ask first
    }

    book.clear();

    // incremental updates
    SECTION("Bid book is sorted in descending order from incremental updates") {
        book.apply_l2_update({ 0, 0, UpdateType::Incremental, BookSide::Bid, 100.0, 100.0 });
        book.apply_l2_update({ 0, 0, UpdateType::Incremental, BookSide::Bid, 99.0, 200.0 });
        book.apply_l2_update({ 0, 0, UpdateType::Incremental, BookSide::Bid, 101.0, 300.0 });

        REQUIRE(book.best_bid() == 101.0);  // Highest bid first
    }

    SECTION("Ask book is sorted in ascending order from incremental updates") {
        book.apply_l2_update({ 0, 0, UpdateType::Incremental, BookSide::Ask, 102.0, 100.0 });
        book.apply_l2_update({ 0, 0, UpdateType::Incremental, BookSide::Ask, 104.0, 200.0 });
        book.apply_l2_update({ 0, 0, UpdateType::Incremental, BookSide::Ask, 103.0, 300.0 });

        REQUIRE(book.best_ask() == 102.0);  // Lowest ask first
    }
}

TEST_CASE("[OrderBook] - Edge Cases", "[orderbook][edge]") {
    OrderBook book;

    SECTION("Zero price handling") {
        REQUIRE_THROWS(book.apply_l2_update({ 0, 0, UpdateType::Snapshot, BookSide::Bid, 0.0, 100.0 }));
    }

    SECTION("Negative price rejection") {
        REQUIRE_THROWS(book.apply_l2_update({ 0, 0, UpdateType::Snapshot, BookSide::Ask, -1.0, 100.0 }));
    }
}