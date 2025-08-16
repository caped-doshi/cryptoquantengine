/*
 * File: tests/test_execution_engine.cpp
 * Description: Unit tests for
 * hftengine/core/execution_engine/execution_engine.cpp.
 * Author: Arvind Rathnashyam
 * Date: 2025-06-27
 * License: Proprietary
 */

#include <catch2/catch_approx.hpp>
#include <catch2/catch_test_macros.hpp>
#include <memory>

#include "core/execution_engine/execution_engine.h"
#include "core/market_data/book_update.h"
#include "core/orderbook/orderbook.h"
#include "core/types/book_side.h"
#include "core/types/order_type.h"
#include "core/types/time_in_force.h"
#include "core/types/update_type.h"
#include "core/types/usings.h"
#include "utils/logger/log_level.h"
#include "utils/logger/logger.h"

TEST_CASE("[ExecutionEngine] - multi-asset handling",
          "[execution-engine][multi-asset]") {
    int asset1 = 0;
    double tick_size_1 = 0.01;
    double lot_size_1 = 0.00001;
    int asset2 = 1;
    double tick_size_2 = 0.01;
    double lot_size_2 = 0.00001;

    auto logger = std::make_shared<Logger>(
        "test_execution_engine_multiasset.log", LogLevel::Debug);
    ExecutionEngine engine(logger);
    engine.add_asset(asset1, tick_size_1, lot_size_1);
    engine.add_asset(asset2, tick_size_2, lot_size_2);

    // Add book updates for two assets
    engine.handle_book_update(
        asset1, BookUpdate{.exch_timestamp_ = 0,
                           .local_timestamp_ = 10,
                           .update_type_ = UpdateType::Incremental,
                           .side_ = BookSide::Ask,
                           .price_ = 101.0,
                           .quantity_ = 10.0});
    engine.handle_book_update(
        asset1, BookUpdate{.exch_timestamp_ = 10,
                           .local_timestamp_ = 20,
                           .update_type_ = UpdateType::Incremental,
                           .side_ = BookSide::Bid,
                           .price_ = 99.0,
                           .quantity_ = 10.0});
    engine.handle_book_update(
        asset2, BookUpdate{.exch_timestamp_ = 20,
                           .local_timestamp_ = 30,
                           .update_type_ = UpdateType::Incremental,
                           .side_ = BookSide::Ask,
                           .price_ = 202.0,
                           .quantity_ = 20.0});
    engine.handle_book_update(
        asset2, BookUpdate{.exch_timestamp_ = 30,
                           .local_timestamp_ = 40,
                           .update_type_ = UpdateType::Incremental,
                           .side_ = BookSide::Bid,
                           .price_ = 198.0,
                           .quantity_ = 20.0});

    SECTION("Submit market buy order for asset 1") {
        auto buy_order =
            std::make_shared<Order>(Order{.exch_timestamp_ = 100,
                                          .orderId_ = 1,
                                          .side_ = BookSide::Bid,
                                          .price_ = 0.0,
                                          .quantity_ = 5.0,
                                          .filled_quantity_ = 0.0,
                                          .tif_ = TimeInForce::GTC,
                                          .orderType_ = OrderType::MARKET,
                                          .queueEst_ = 0.0});

        engine.execute_market_order(asset1, TradeSide::Buy, buy_order);

        auto fills = engine.fills();
        REQUIRE(fills.size() == 1);
        REQUIRE(fills[0].asset_id_ == asset1);
        REQUIRE(fills[0].price_ == 101.0);
        REQUIRE(fills[0].quantity_ == 5.0);
    }

    SECTION("Submit market sell order for asset 2") {
        engine.clear_fills();

        auto sell_order =
            std::make_shared<Order>(Order{.exch_timestamp_ = 200,
                                          .orderId_ = 2,
                                          .side_ = BookSide::Ask,
                                          .price_ = 0.0,
                                          .quantity_ = 10.0,
                                          .filled_quantity_ = 0.0,
                                          .tif_ = TimeInForce::GTC,
                                          .orderType_ = OrderType::LIMIT,
                                          .queueEst_ = 0.0});

        engine.execute_market_order(asset2, TradeSide::Sell, sell_order);

        auto fills = engine.fills();
        REQUIRE(fills.size() == 1);
        REQUIRE(fills[0].asset_id_ == asset2);
        REQUIRE(fills[0].price_ == 198.0);
        REQUIRE(fills[0].quantity_ == 10.0);
    }

    SECTION("Ensure asset1 orders do not affect asset2") {
        auto order1 =
            std::make_shared<Order>(Order{.exch_timestamp_ = 300,
                                          .orderId_ = 3,
                                          .side_ = BookSide::Bid,
                                          .price_ = 100.0,
                                          .quantity_ = 3.0,
                                          .filled_quantity_ = 0.0,
                                          .tif_ = TimeInForce::GTC,
                                          .orderType_ = OrderType::LIMIT,
                                          .queueEst_ = 0.0});

        auto order2 =
            std::make_shared<Order>(Order{.exch_timestamp_ = 301,
                                          .orderId_ = 4,
                                          .side_ = BookSide::Bid,
                                          .price_ = 200.0,
                                          .quantity_ = 4.0,
                                          .filled_quantity_ = 0.0,
                                          .tif_ = TimeInForce::GTC,
                                          .orderType_ = OrderType::LIMIT,
                                          .queueEst_ = 0.0});

        engine.place_maker_order(asset1, order1);
        engine.place_maker_order(asset2, order2);
    }
}

TEST_CASE("[ExecutionEngine] - cancel_order", "[execution][cancel]") {
    auto logger = std::make_shared<Logger>("test_execution_engine_cancel.log",
                                           LogLevel::Debug);
    ExecutionEngine engine(logger);

    const int asset_id = 1;
    const double tick_size = 0.01;
    const double lot_size = 0.00001;
    engine.add_asset(asset_id, tick_size, lot_size);

    // Create and submit a GTC order
    Order order{.exch_timestamp_ = 1000,
                .orderId_ = 123456789,
                .side_ = BookSide::Bid,
                .price_ = 100.0,
                .quantity_ = 1.0,
                .filled_quantity_ = 0.0,
                .tif_ = TimeInForce::GTC,
                .orderType_ = OrderType::LIMIT,
                .queueEst_ = 0.0};

    auto order_ptr = std::make_shared<Order>(order);

    // Manually place GTC order (simulate submission logic)
    bool placed = engine.place_maker_order(asset_id, order_ptr);
    REQUIRE(placed);

    // Order should be present before cancellation
    REQUIRE(engine.order_exists(order.orderId_)); // Optional helper function

    // Now cancel the order
    bool cancelled = engine.cancel_order(asset_id, order.orderId_, 1100);
    engine.clear_inactive_orders(asset_id);

    // Order should no longer exist
    REQUIRE(cancelled);
    REQUIRE(engine.order_exists(order.orderId_) == false);
}

TEST_CASE("[ExecutionEngine] - executes market buy and sell orders",
          "[execution-engine][market]") {
    auto logger = std::make_shared<Logger>("test_execution_engine_market.log",
                                           LogLevel::Debug);
    ExecutionEngine engine(logger);

    const int asset_id = 1;
    const double tick_size = 0.01;
    const double lot_size = 0.00001;
    engine.add_asset(asset_id, tick_size, lot_size);

    engine.handle_book_update(
        asset_id, BookUpdate{.exch_timestamp_ = 0,
                             .local_timestamp_ = 10,
                             .update_type_ = UpdateType::Incremental,
                             .side_ = BookSide::Ask,
                             .price_ = 101.0,
                             .quantity_ = 2.0});
    engine.handle_book_update(
        asset_id, BookUpdate{.exch_timestamp_ = 10,
                             .local_timestamp_ = 20,
                             .update_type_ = UpdateType::Incremental,
                             .side_ = BookSide::Ask,
                             .price_ = 102.0,
                             .quantity_ = 3.0});
    engine.handle_book_update(
        asset_id, BookUpdate{.exch_timestamp_ = 20,
                             .local_timestamp_ = 30,
                             .update_type_ = UpdateType::Incremental,
                             .side_ = BookSide::Bid,
                             .price_ = 100.0,
                             .quantity_ = 1.5});
    engine.handle_book_update(
        asset_id, BookUpdate{.exch_timestamp_ = 30,
                             .local_timestamp_ = 40,
                             .update_type_ = UpdateType::Incremental,
                             .side_ = BookSide::Bid,
                             .price_ = 99.0,
                             .quantity_ = 1.0});

    SECTION("Market buy order fills against ask levels") {
        auto buy_order =
            std::make_shared<Order>(Order{.local_timestamp_ = 50,
                                          .exch_timestamp_ = 60,
                                          .orderId_ = 1,
                                          .price_ = 0.0,
                                          .quantity_ = 4.0,
                                          .filled_quantity_ = 0.0,
                                          .tif_ = TimeInForce::GTC,
                                          .orderType_ = OrderType::MARKET});

        engine.execute_market_order(asset_id, TradeSide::Buy, buy_order);

        const auto &fills = engine.fills();
        REQUIRE(buy_order->filled_quantity_ == 4.0);
        REQUIRE(fills.size() == 2);
        REQUIRE(fills[0].price_ == 101.0);
        REQUIRE(fills[0].quantity_ == 2.0);
        REQUIRE(fills[0].side_ == TradeSide::Buy);
        REQUIRE(fills[1].price_ == 102.0);
        REQUIRE(fills[1].quantity_ == 2.0);
        REQUIRE(fills[1].side_ == TradeSide::Buy);
    }

    SECTION("Market sell order fills against bid levels") {
        auto sell_order =
            std::make_shared<Order>(Order{.local_timestamp_ = 50,
                                          .exch_timestamp_ = 60,
                                          .orderId_ = 2,
                                          .price_ = 0.0,
                                          .quantity_ = 2.0,
                                          .filled_quantity_ = 0.0,
                                          .tif_ = TimeInForce::GTC,
                                          .orderType_ = OrderType::MARKET});

        engine.execute_market_order(asset_id, TradeSide::Sell, sell_order);

        const auto &fills = engine.fills();
        REQUIRE(sell_order->filled_quantity_ == 2.0);
        REQUIRE(fills.size() == 2);
        REQUIRE(fills[0].price_ == 100.0);
        REQUIRE(fills[0].quantity_ == 1.5);
        REQUIRE(fills[0].side_ == TradeSide::Sell);
        REQUIRE(fills[1].price_ == 99.0);
        REQUIRE(fills[1].quantity_ == 0.5);
        REQUIRE(fills[1].side_ == TradeSide::Sell);
    }
}

TEST_CASE("[ExecutionEngine] - executes limit fill-or-kill buy and sell orders",
          "[execution-engine][FOK]") {
    const int asset_id = 1;
    const double tick_size = 0.01;
    const double lot_size = 0.00001;

    auto logger = std::make_shared<Logger>("test_execution_engine_FOK.log",
                                           LogLevel::Debug);
    ExecutionEngine engine(logger);
    engine.add_asset(asset_id, tick_size, lot_size);

    engine.handle_book_update(
        asset_id, BookUpdate{.exch_timestamp_ = 0,
                             .local_timestamp_ = 10,
                             .update_type_ = UpdateType::Incremental,
                             .side_ = BookSide::Ask,
                             .price_ = 101.0,
                             .quantity_ = 2.0});
    engine.handle_book_update(
        asset_id, BookUpdate{.exch_timestamp_ = 10,
                             .local_timestamp_ = 20,
                             .update_type_ = UpdateType::Incremental,
                             .side_ = BookSide::Ask,
                             .price_ = 102.0,
                             .quantity_ = 3.0});
    engine.handle_book_update(
        asset_id, BookUpdate{.exch_timestamp_ = 20,
                             .local_timestamp_ = 30,
                             .update_type_ = UpdateType::Incremental,
                             .side_ = BookSide::Bid,
                             .price_ = 100.0,
                             .quantity_ = 1.5});
    engine.handle_book_update(
        asset_id, BookUpdate{.exch_timestamp_ = 30,
                             .local_timestamp_ = 40,
                             .update_type_ = UpdateType::Incremental,
                             .side_ = BookSide::Bid,
                             .price_ = 99.0,
                             .quantity_ = 1.0});

    SECTION("Limit buy fill-or-kill order fills against ask levels") {
        auto buy_order =
            std::make_shared<Order>(Order{.local_timestamp_ = 50,
                                          .exch_timestamp_ = 60,
                                          .orderId_ = 1,
                                          .price_ = 101.5,
                                          .quantity_ = 3.0,
                                          .filled_quantity_ = 0.0,
                                          .tif_ = TimeInForce::FOK,
                                          .orderType_ = OrderType::LIMIT});
        REQUIRE(engine.execute_fok_order(asset_id, TradeSide::Buy, buy_order) ==
                false);

        const auto &fills = engine.fills();
        REQUIRE(buy_order->filled_quantity_ == 0.0);
        REQUIRE(fills.size() == 0);
    }
    SECTION("Limit sell fill-or-kill order fills against bid levels") {
        auto sell_order =
            std::make_shared<Order>(Order{.local_timestamp_ = 50,
                                          .exch_timestamp_ = 60,
                                          .orderId_ = 2,
                                          .price_ = 99.0,
                                          .quantity_ = 2.0,
                                          .filled_quantity_ = 0.0,
                                          .tif_ = TimeInForce::FOK,
                                          .orderType_ = OrderType::LIMIT});
        REQUIRE(engine.execute_fok_order(asset_id, TradeSide::Sell,
                                         sell_order) == true);

        const auto &fills = engine.fills();
        REQUIRE(sell_order->filled_quantity_ == 2.0);
        REQUIRE(fills.size() == 2);
        REQUIRE(fills[0].price_ == 100.0);
        REQUIRE(fills[0].quantity_ == 1.5);
        REQUIRE(fills[1].price_ == 99.0);
        REQUIRE(fills[1].quantity_ == 0.5);
    }
}

TEST_CASE(
    "[ExecutionEngine] - executes limit immediate-or-cancel buy and sell ",
    "[execution-engine][IOC]") {
    const int asset_id = 1;
    const double tick_size = 0.01;
    const double lot_size = 0.00001;

    auto logger = std::make_shared<Logger>("test_execution_engine_IOC.log",
                                           LogLevel::Debug);
    ExecutionEngine engine(logger);
    engine.add_asset(asset_id, tick_size, lot_size);

    engine.handle_book_update(
        asset_id, BookUpdate{.exch_timestamp_ = 0,
                             .local_timestamp_ = 10,
                             .update_type_ = UpdateType::Incremental,
                             .side_ = BookSide::Ask,
                             .price_ = 101.0,
                             .quantity_ = 2.0});
    engine.handle_book_update(
        asset_id, BookUpdate{.exch_timestamp_ = 10,
                             .local_timestamp_ = 20,
                             .update_type_ = UpdateType::Incremental,
                             .side_ = BookSide::Ask,
                             .price_ = 102.0,
                             .quantity_ = 3.0});
    engine.handle_book_update(
        asset_id, BookUpdate{.exch_timestamp_ = 20,
                             .local_timestamp_ = 30,
                             .update_type_ = UpdateType::Incremental,
                             .side_ = BookSide::Bid,
                             .price_ = 100.0,
                             .quantity_ = 1.5});
    engine.handle_book_update(
        asset_id, BookUpdate{.exch_timestamp_ = 30,
                             .local_timestamp_ = 40,
                             .update_type_ = UpdateType::Incremental,
                             .side_ = BookSide::Bid,
                             .price_ = 99.0,
                             .quantity_ = 1.0});

    SECTION("Limit buy immediate-or-cancel order fills against ask levels") {
        auto buy_order =
            std::make_shared<Order>(Order{.local_timestamp_ = 50,
                                          .exch_timestamp_ = 60,
                                          .orderId_ = 1,
                                          .price_ = 101.5,
                                          .quantity_ = 3.0,
                                          .filled_quantity_ = 0.0,
                                          .tif_ = TimeInForce::IOC,
                                          .orderType_ = OrderType::LIMIT});
        REQUIRE(engine.execute_ioc_order(asset_id, TradeSide::Buy, buy_order) ==
                true);

        const auto &fills = engine.fills();
        REQUIRE(buy_order->filled_quantity_ == 2.0);
        REQUIRE(fills.size() == 1);
        REQUIRE(fills[0].price_ == 101.0);
        REQUIRE(fills[0].quantity_ == 2.0);
    }
    SECTION("Limit sell immediate-or-cancel order fills against bid levels") {
        auto sell_order =
            std::make_shared<Order>(Order{.local_timestamp_ = 50,
                                          .exch_timestamp_ = 60,
                                          .orderId_ = 2,
                                          .price_ = 99.0,
                                          .quantity_ = 2.0,
                                          .filled_quantity_ = 0.0,
                                          .tif_ = TimeInForce::IOC,
                                          .orderType_ = OrderType::LIMIT});
        REQUIRE(engine.execute_ioc_order(asset_id, TradeSide::Sell,
                                         sell_order) == true);

        const auto &fills = engine.fills();
        REQUIRE(sell_order->filled_quantity_ == 2.0);
        REQUIRE(fills.size() == 2);
        REQUIRE(fills[0].price_ == 100.0);
        REQUIRE(fills[0].quantity_ == 1.5);
        REQUIRE(fills[1].price_ == 99.0);
        REQUIRE(fills[1].quantity_ == 0.5);
    }
}

TEST_CASE("[ExecutionEngine] - places limit GTC orders ",
          "[execution-engine][GTC]") {
    const int asset_id = 1;
    const double tick_size = 0.01;
    const double lot_size = 0.00001;

    auto logger = std::make_shared<Logger>("test_execution_engine_gtc.log",
                                           LogLevel::Debug);
    ExecutionEngine engine(logger);
    engine.add_asset(asset_id, tick_size, lot_size);

    engine.handle_book_update(
        asset_id, BookUpdate{.exch_timestamp_ = 0,
                             .local_timestamp_ = 10,
                             .update_type_ = UpdateType::Incremental,
                             .side_ = BookSide::Ask,
                             .price_ = 101.0,
                             .quantity_ = 2.0});
    engine.handle_book_update(
        asset_id, BookUpdate{.exch_timestamp_ = 10,
                             .local_timestamp_ = 20,
                             .update_type_ = UpdateType::Incremental,
                             .side_ = BookSide::Ask,
                             .price_ = 102.0,
                             .quantity_ = 3.0});
    engine.handle_book_update(
        asset_id, BookUpdate{.exch_timestamp_ = 20,
                             .local_timestamp_ = 30,
                             .update_type_ = UpdateType::Incremental,
                             .side_ = BookSide::Bid,
                             .price_ = 100.0,
                             .quantity_ = 1.5});
    engine.handle_book_update(
        asset_id, BookUpdate{.exch_timestamp_ = 30,
                             .local_timestamp_ = 40,
                             .update_type_ = UpdateType::Incremental,
                             .side_ = BookSide::Bid,
                             .price_ = 99.0,
                             .quantity_ = 1.0});

    SECTION("Places limit GTC buy order against bid levels") {
        auto buy_order_1 =
            std::make_shared<Order>(Order{.local_timestamp_ = 50,
                                          .exch_timestamp_ = 60,
                                          .orderId_ = 1,
                                          .side_ = BookSide::Bid,
                                          .price_ = 101.5,
                                          .quantity_ = 3.0,
                                          .filled_quantity_ = 0.0,
                                          .tif_ = TimeInForce::GTC,
                                          .orderType_ = OrderType::LIMIT,
                                          .queueEst_ = 0.0});
        REQUIRE(engine.place_maker_order(asset_id, buy_order_1) == false);
        const auto &fills_1 = engine.fills();
        REQUIRE(fills_1.size() == 0);

        auto buy_order_2 =
            std::make_shared<Order>(Order{.local_timestamp_ = 50,
                                          .exch_timestamp_ = 60,
                                          .orderId_ = 1,
                                          .side_ = BookSide::Bid,
                                          .price_ = 98.0,
                                          .quantity_ = 3.0,
                                          .filled_quantity_ = 0.0,
                                          .tif_ = TimeInForce::GTC,
                                          .orderType_ = OrderType::LIMIT,
                                          .queueEst_ = 0.0});
        REQUIRE(engine.place_maker_order(asset_id, buy_order_2) == true);
        const auto &fills_2 = engine.fills();
        REQUIRE(fills_2.size() == 0);
    }
    SECTION("Places limit GTC sell order against ask levels") {
        auto sell_order_1 =
            std::make_shared<Order>(Order{.local_timestamp_ = 50,
                                          .exch_timestamp_ = 60,
                                          .orderId_ = 2,
                                          .side_ = BookSide::Ask,
                                          .price_ = 99.0,
                                          .quantity_ = 2.0,
                                          .filled_quantity_ = 0.0,
                                          .tif_ = TimeInForce::GTC,
                                          .orderType_ = OrderType::LIMIT});
        REQUIRE(engine.place_maker_order(asset_id, sell_order_1) == false);
        const auto &fills_1 = engine.fills();
        REQUIRE(fills_1.size() == 0);

        auto sell_order_2 =
            std::make_shared<Order>(Order{.local_timestamp_ = 50,
                                          .exch_timestamp_ = 60,
                                          .orderId_ = 2,
                                          .side_ = BookSide::Ask,
                                          .price_ = 102.0,
                                          .quantity_ = 2.0,
                                          .filled_quantity_ = 0.0,
                                          .tif_ = TimeInForce::GTC,
                                          .orderType_ = OrderType::LIMIT});
        REQUIRE(engine.place_maker_order(asset_id, sell_order_2) == true);
        REQUIRE(sell_order_2->queueEst_ == 3.0);
        const auto &fills_2 = engine.fills();
        REQUIRE(fills_2.size() == 0);
    }
}

TEST_CASE("[ExecutionEngine] - submit_order routes correctly",
          "[execution-engine][routing]") {
    const int asset_id = 1;
    const double tick_size = 0.01;
    const double lot_size = 0.00001;

    auto logger = std::make_shared<Logger>("test_execution_engine_routing.log",
                                           LogLevel::Debug);
    ExecutionEngine engine(logger);
    engine.add_asset(asset_id, tick_size, lot_size);

    engine.handle_book_update(
        asset_id, BookUpdate{.exch_timestamp_ = 0,
                             .local_timestamp_ = 10,
                             .update_type_ = UpdateType::Incremental,
                             .side_ = BookSide::Ask,
                             .price_ = 101.0,
                             .quantity_ = 2.0});
    engine.handle_book_update(
        asset_id, BookUpdate{.exch_timestamp_ = 10,
                             .local_timestamp_ = 20,
                             .update_type_ = UpdateType::Incremental,
                             .side_ = BookSide::Ask,
                             .price_ = 102.0,
                             .quantity_ = 3.0});
    engine.handle_book_update(
        asset_id, BookUpdate{.exch_timestamp_ = 20,
                             .local_timestamp_ = 30,
                             .update_type_ = UpdateType::Incremental,
                             .side_ = BookSide::Bid,
                             .price_ = 100.0,
                             .quantity_ = 1.5});
    engine.handle_book_update(
        asset_id, BookUpdate{.exch_timestamp_ = 30,
                             .local_timestamp_ = 40,
                             .update_type_ = UpdateType::Incremental,
                             .side_ = BookSide::Bid,
                             .price_ = 99.0,
                             .quantity_ = 1.0});

    SECTION("Market buy order is routed to execute_market_order") {
        Order market_buy_order{.local_timestamp_ = 50,
                               .exch_timestamp_ = 60,
                               .orderId_ = 1,
                               .price_ = 0.0,
                               .quantity_ = 4.0,
                               .filled_quantity_ = 0.0,
                               .tif_ = TimeInForce::GTC,
                               .orderType_ = OrderType::MARKET};
        REQUIRE_NOTHROW(
            engine.execute_order(asset_id, TradeSide::Buy, market_buy_order));
        const auto &fills = engine.fills();
        REQUIRE(fills.size() == 2);
    }
    SECTION("Market sell order is routed to execute_market_order") {
        Order market_sell_order{.local_timestamp_ = 50,
                                .exch_timestamp_ = 60,
                                .orderId_ = 2,
                                .price_ = 0.0,
                                .quantity_ = 2.0,
                                .filled_quantity_ = 0.0,
                                .tif_ = TimeInForce::GTC,
                                .orderType_ = OrderType::MARKET};
        REQUIRE_NOTHROW(
            engine.execute_order(asset_id, TradeSide::Sell, market_sell_order));
        const auto &fills = engine.fills();
        REQUIRE(fills.size() == 2);
    }

    SECTION("Limit buy order with FOK is routed to execute_fok_order") {
        Order limit_fok_buy_order{.local_timestamp_ = 50,
                                  .exch_timestamp_ = 60,
                                  .orderId_ = 1,
                                  .price_ = 101.5,
                                  .quantity_ = 3.0,
                                  .filled_quantity_ = 0.0,
                                  .tif_ = TimeInForce::FOK,
                                  .orderType_ = OrderType::LIMIT};
        REQUIRE_NOTHROW(engine.execute_order(asset_id, TradeSide::Buy,
                                             limit_fok_buy_order));
        const auto &fills = engine.fills();
        REQUIRE(fills.size() == 0);
    }

    SECTION("Limit sell order with FOK is routed to execute_fok_order") {
        Order limit_fok_sell_order{.local_timestamp_ = 50,
                                   .exch_timestamp_ = 60,
                                   .orderId_ = 2,
                                   .price_ = 99.0,
                                   .quantity_ = 2.0,
                                   .filled_quantity_ = 0.0,
                                   .tif_ = TimeInForce::FOK,
                                   .orderType_ = OrderType::LIMIT};
        REQUIRE_NOTHROW(engine.execute_order(asset_id, TradeSide::Sell,
                                             limit_fok_sell_order));

        const auto &fills = engine.fills();
        REQUIRE(fills.size() == 2);
    }

    SECTION("Limit buy order with IOC is routed to execute_ioc_order") {
        Order limit_ioc_buy_order{.local_timestamp_ = 50,
                                  .exch_timestamp_ = 60,
                                  .orderId_ = 1,
                                  .price_ = 101.5,
                                  .quantity_ = 3.0,
                                  .filled_quantity_ = 0.0,
                                  .tif_ = TimeInForce::IOC,
                                  .orderType_ = OrderType::LIMIT};
        REQUIRE_NOTHROW(engine.execute_order(asset_id, TradeSide::Buy,
                                             limit_ioc_buy_order));
        const auto &fills = engine.fills();
        REQUIRE(fills.size() == 1);
    }

    SECTION("Limit sell order with IOC is routed to execute_ioc_order") {
        Order limit_ioc_sell_order{.local_timestamp_ = 50,
                                   .exch_timestamp_ = 60,
                                   .orderId_ = 2,
                                   .price_ = 99.0,
                                   .quantity_ = 2.0,
                                   .filled_quantity_ = 0.0,
                                   .tif_ = TimeInForce::IOC,
                                   .orderType_ = OrderType::LIMIT};
        REQUIRE_NOTHROW(engine.execute_order(asset_id, TradeSide::Sell,
                                             limit_ioc_sell_order));
        const auto &fills = engine.fills();
        REQUIRE(fills.size() == 2);
    }

    SECTION("Limit bid order with GTC is routed to place_maker_order") {
        Order limit_gtx_buy_order{.local_timestamp_ = 50,
                                  .exch_timestamp_ = 60,
                                  .orderId_ = 1,
                                  .side_ = BookSide::Bid,
                                  .price_ = 101.5,
                                  .quantity_ = 3.0,
                                  .filled_quantity_ = 0.0,
                                  .tif_ = TimeInForce::GTC,
                                  .orderType_ = OrderType::LIMIT,
                                  .queueEst_ = 0.0};
        REQUIRE_NOTHROW(engine.execute_order(asset_id, TradeSide::Buy,
                                             limit_gtx_buy_order));
        const auto &fills = engine.fills();
        REQUIRE(fills.size() == 0);
    }

    SECTION("Limit ask order with GTC is routed to place_maker_order") {
        Order limit_gtx_sell_order{.local_timestamp_ = 50,
                                   .exch_timestamp_ = 60,
                                   .orderId_ = 2,
                                   .side_ = BookSide::Ask,
                                   .price_ = 99.0,
                                   .quantity_ = 2.0,
                                   .filled_quantity_ = 0.0,
                                   .tif_ = TimeInForce::GTC,
                                   .orderType_ = OrderType::LIMIT};
        REQUIRE(engine.execute_order(asset_id, TradeSide::Sell,
                                     limit_gtx_sell_order) == false);

        const auto &fills = engine.fills();
        REQUIRE(fills.size() == 0);
    }

    SECTION("Unsupported TIF throws exception") {
        Order invalid_order{
            .local_timestamp_ = 100,
            .exch_timestamp_ = 110,
            .orderId_ = 1238,
            .side_ = BookSide::Ask,
            .price_ = 50200.0,
            .quantity_ = 1.5,
            .filled_quantity_ = 0.0,
            .tif_ = static_cast<TimeInForce>(999), // Invalid enum value
            .orderType_ = OrderType::LIMIT,
            .queueEst_ = 0.0};
        REQUIRE_THROWS_AS(
            engine.execute_order(asset_id, TradeSide::Sell, invalid_order),
            std::invalid_argument);
    }
}

TEST_CASE("[ExecutionEngine] - queue estimation", "[execution-engine][queue]") {
    int asset_id = 0;
    double tick_size = 0.01;
    double lot_size = 0.00001;

    auto logger = std::make_shared<Logger>("test_execution_engine_queue.log",
                                           LogLevel::Debug);
    ExecutionEngine engine(logger);
    engine.add_asset(asset_id, tick_size, lot_size);

    engine.handle_book_update(
        0, BookUpdate{.exch_timestamp_ = 10,
                      .local_timestamp_ = 20,
                      .update_type_ = UpdateType::Incremental,
                      .side_ = BookSide::Ask,
                      .price_ = 102.0,
                      .quantity_ = 3.0});
    engine.handle_book_update(
        0, BookUpdate{.exch_timestamp_ = 30,
                      .local_timestamp_ = 40,
                      .update_type_ = UpdateType::Incremental,
                      .side_ = BookSide::Bid,
                      .price_ = 99.0,
                      .quantity_ = 1.0});

    auto buy_order_1 =
        std::make_shared<Order>(Order{.local_timestamp_ = 50,
                                      .exch_timestamp_ = 60,
                                      .orderId_ = 1,
                                      .side_ = BookSide::Bid,
                                      .price_ = 99.0,
                                      .quantity_ = 3.0,
                                      .filled_quantity_ = 0.0,
                                      .tif_ = TimeInForce::GTC,
                                      .orderType_ = OrderType::LIMIT,
                                      .queueEst_ = 0.0});
    auto sell_order_1 =
        std::make_shared<Order>(Order{.local_timestamp_ = 50,
                                      .exch_timestamp_ = 60,
                                      .orderId_ = 2,
                                      .side_ = BookSide::Ask,
                                      .price_ = 102.0,
                                      .quantity_ = 1.0,
                                      .filled_quantity_ = 0.0,
                                      .tif_ = TimeInForce::GTC,
                                      .orderType_ = OrderType::LIMIT,
                                      .queueEst_ = 0.0});

    SECTION("Initial queue estimation") {
        // placed at the end of existing depth at price level
        engine.place_maker_order(0, buy_order_1);
        engine.place_maker_order(0, sell_order_1);
        REQUIRE(buy_order_1->queueEst_ == 1.0);
        REQUIRE(sell_order_1->queueEst_ == 3.0);
    }
    SECTION("Queue estimation updates") {
        // f(x) = ln(1+x)
        engine.place_maker_order(0, buy_order_1);
        engine.place_maker_order(0, sell_order_1);

        engine.handle_book_update(
            0, BookUpdate{.exch_timestamp_ = 60,
                          .local_timestamp_ = 70,
                          .update_type_ = UpdateType::Incremental,
                          .side_ = BookSide::Bid,
                          .price_ = 99.0,
                          .quantity_ = 0.2});

        engine.handle_book_update(
            0, BookUpdate{.exch_timestamp_ = 70,
                          .local_timestamp_ = 80,
                          .update_type_ = UpdateType::Incremental,
                          .side_ = BookSide::Ask,
                          .price_ = 102.0,
                          .quantity_ = 5.0});
        // buy order at bottom of queue, queue est. should decrease by delta
        REQUIRE(buy_order_1->queueEst_ == Catch::Approx(0.2).margin(1e-8));
        // sell order position remains unchanged
        REQUIRE(sell_order_1->queueEst_ == Catch::Approx(3.0).margin(1e-8));

        engine.handle_book_update(
            0, BookUpdate{.exch_timestamp_ = 80,
                          .local_timestamp_ = 90,
                          .update_type_ = UpdateType::Incremental,
                          .side_ = BookSide::Ask,
                          .price_ = 102.0,
                          .quantity_ = 1.0});
        REQUIRE(sell_order_1->queueEst_ ==
                Catch::Approx(1.0 / 3.0).margin(1e-8));
        engine.handle_book_update(
            0, BookUpdate{.exch_timestamp_ = 90,
                          .local_timestamp_ = 100,
                          .update_type_ = UpdateType::Incremental,
                          .side_ = BookSide::Ask,
                          .price_ = 102.0,
                          .quantity_ = 0.5});
        REQUIRE(sell_order_1->queueEst_ == 0.0);
    }
}

TEST_CASE("[ExecutionEngine] - processes trades", "[execution-engine][trade]") {
    SECTION("Full execution of bid order when sufficient trade size") {
        int asset_id = 0;
        double tick_size = 0.01;
        double lot_size = 0.00001;

        auto logger = std::make_shared<Logger>(
            "test_execution_engine_trade_s1.log", LogLevel::Debug);
        ExecutionEngine engine(logger);
        engine.add_asset(asset_id, tick_size, lot_size);

        auto buy_order =
            std::make_shared<Order>(Order{.exch_timestamp_ = 10,
                                          .orderId_ = 1,
                                          .side_ = BookSide::Bid,
                                          .price_ = 100.0,
                                          .quantity_ = 1.0,
                                          .filled_quantity_ = 0.0,
                                          .tif_ = TimeInForce::GTC,
                                          .orderType_ = OrderType::LIMIT,
                                          .queueEst_ = 0.0});
        engine.place_maker_order(0, buy_order);
        REQUIRE(buy_order->queueEst_ == 0.0);

        engine.handle_trade(asset_id, Trade{.exch_timestamp_ = 20,
                                            .local_timestamp_ = 25,
                                            .side_ = TradeSide::Sell,
                                            .price_ = 100.0,
                                            .quantity_ = 1.0,
                                            .orderId_ = 999});

        REQUIRE(buy_order->filled_quantity_ == buy_order->quantity_);

        const auto &fills = engine.fills();
        REQUIRE(fills.size() == 1);
        REQUIRE(fills[0].price_ == 100.0);
        REQUIRE(fills[0].quantity_ == 1.0);
        REQUIRE(fills[0].side_ == TradeSide::Buy);
        REQUIRE(fills[0].orderId_ == 1);
        REQUIRE(fills[0].is_maker == true);
        REQUIRE(buy_order->filled_quantity_ == 1.0);

        engine.clear_fills();
        REQUIRE(fills.size() == 0);

        auto sell_order =
            std::make_shared<Order>(Order{.exch_timestamp_ = 30,
                                          .orderId_ = 2,
                                          .side_ = BookSide::Ask,
                                          .price_ = 102.0,
                                          .quantity_ = 1.0,
                                          .filled_quantity_ = 0.0,
                                          .tif_ = TimeInForce::GTC,
                                          .orderType_ = OrderType::LIMIT,
                                          .queueEst_ = 0.0});
        engine.place_maker_order(asset_id, sell_order);

        engine.handle_trade(asset_id, Trade{.exch_timestamp_ = 40,
                                            .local_timestamp_ = 45,
                                            .side_ = TradeSide::Buy,
                                            .price_ = 102.0,
                                            .quantity_ = 3.0,
                                            .orderId_ = 999});
        REQUIRE(fills.size() == 1);
        REQUIRE(fills[0].price_ == 102.0);
        REQUIRE(fills[0].quantity_ == 1.0);
        REQUIRE(fills[0].side_ == TradeSide::Sell);
        REQUIRE(fills[0].orderId_ == 2);
        REQUIRE(fills[0].is_maker == true);
        REQUIRE(buy_order->filled_quantity_ == 1.0);
    }
    SECTION("Partially fills order when trade quantity is smaller") {
        int asset_id = 0;
        double tick_size = 0.01;
        double lot_size = 0.00001;

        auto logger = std::make_shared<Logger>(
            "test_execution_engine_trade_s1.log", LogLevel::Debug);
        ExecutionEngine engine(logger);
        engine.add_asset(asset_id, tick_size, lot_size);

        auto buy_order =
            std::make_shared<Order>(Order{.exch_timestamp_ = 10,
                                          .orderId_ = 1,
                                          .side_ = BookSide::Bid,
                                          .price_ = 101.0,
                                          .quantity_ = 1.5,
                                          .filled_quantity_ = 0.0,
                                          .tif_ = TimeInForce::GTC,
                                          .orderType_ = OrderType::LIMIT,
                                          .queueEst_ = 0.0});
        engine.place_maker_order(asset_id, buy_order);
        engine.handle_trade(asset_id, Trade{.exch_timestamp_ = 20,
                                            .local_timestamp_ = 21,
                                            .side_ = TradeSide::Sell,
                                            .price_ = 101.0,
                                            .quantity_ = 1.0,
                                            .orderId_ = 1000});

        REQUIRE(buy_order->filled_quantity_ == 1.0);
        REQUIRE(buy_order->queueEst_ == 0.0);
        REQUIRE(engine.fills().size() == 1);

        engine.clear_fills();
        REQUIRE(engine.fills().size() == 0);

        auto sell_order =
            std::make_shared<Order>(Order{.exch_timestamp_ = 30,
                                          .orderId_ = 1,
                                          .side_ = BookSide::Ask,
                                          .price_ = 103.0,
                                          .quantity_ = 3.5,
                                          .filled_quantity_ = 0.0,
                                          .tif_ = TimeInForce::GTC,
                                          .orderType_ = OrderType::LIMIT,
                                          .queueEst_ = 0.0});
        engine.place_maker_order(asset_id, sell_order);
        engine.handle_trade(asset_id, Trade{.exch_timestamp_ = 40,
                                            .local_timestamp_ = 41,
                                            .side_ = TradeSide::Buy,
                                            .price_ = 103.0,
                                            .quantity_ = 2.0,
                                            .orderId_ = 1000});

        REQUIRE(sell_order->filled_quantity_ == 2.0);
        REQUIRE(sell_order->queueEst_ == 0.0);
        REQUIRE(engine.fills().size() == 1);
    }
    SECTION("Does not fill if trade timestamp is before order") {
        int asset_id = 0;
        double tick_size = 0.01;
        double lot_size = 0.00001;

        auto logger = std::make_shared<Logger>(
            "test_execution_engine_trade_s3.log", LogLevel::Debug);
        ExecutionEngine engine(logger);
        engine.add_asset(asset_id, tick_size, lot_size);

        auto buy_order =
            std::make_shared<Order>(Order{.exch_timestamp_ = 30,
                                          .orderId_ = 2,
                                          .side_ = BookSide::Bid,
                                          .price_ = 101.0,
                                          .quantity_ = 2.0,
                                          .filled_quantity_ = 0.0,
                                          .tif_ = TimeInForce::GTC,
                                          .orderType_ = OrderType::LIMIT,
                                          .queueEst_ = 0.0});
        engine.place_maker_order(asset_id, buy_order);
        engine.handle_trade(asset_id, Trade{.exch_timestamp_ = 20,
                                            .local_timestamp_ = 21,
                                            .side_ = TradeSide::Sell,
                                            .price_ = 101.0,
                                            .quantity_ = 1.0,
                                            .orderId_ = 1234});

        REQUIRE(buy_order->filled_quantity_ == 0.0);
        REQUIRE(engine.fills().empty());

        auto sell_order =
            std::make_shared<Order>(Order{.exch_timestamp_ = 60,
                                          .orderId_ = 2,
                                          .side_ = BookSide::Ask,
                                          .price_ = 101.0,
                                          .quantity_ = 2.0,
                                          .filled_quantity_ = 0.0,
                                          .tif_ = TimeInForce::GTC,
                                          .orderType_ = OrderType::LIMIT,
                                          .queueEst_ = 0.0});
        engine.place_maker_order(asset_id, sell_order);
        engine.handle_trade(asset_id, Trade{.exch_timestamp_ = 40,
                                            .local_timestamp_ = 21,
                                            .side_ = TradeSide::Buy,
                                            .price_ = 101.0,
                                            .quantity_ = 1.0,
                                            .orderId_ = 1234});

        REQUIRE(buy_order->filled_quantity_ == 0.0);
        REQUIRE(engine.fills().empty());
    }
}