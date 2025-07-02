/*
 * File: tests/test_execution_engine.cpp
 * Description: Unit tests for
 * hftengine/core/execution_engine/execution_engine.cpp. Author: Arvind
 * Rathnashyam Date: 2025-06-27 License: Proprietary
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

TEST_CASE("[ExecutionEngine] - multi-asset handling",
          "[execution-engine][multi-asset]") {
    ExecutionEngine engine;

    int asset1 = 0;
    int asset2 = 1;

    // Add book updates for two assets
    engine.handle_book_update(
        asset1, BookUpdate{.timestamp_ = 0,
                           .localTimestamp_ = 10,
                           .update_type_ = UpdateType::Incremental,
                           .side_ = BookSide::Ask,
                           .price_ = 101.0,
                           .quantity_ = 10.0});
    engine.handle_book_update(
        asset1, BookUpdate{.timestamp_ = 10,
                           .localTimestamp_ = 20,
                           .update_type_ = UpdateType::Incremental,
                           .side_ = BookSide::Bid,
                           .price_ = 99.0,
                           .quantity_ = 10.0});
    engine.handle_book_update(
        asset2, BookUpdate{.timestamp_ = 20,
                           .localTimestamp_ = 30,
                           .update_type_ = UpdateType::Incremental,
                           .side_ = BookSide::Ask,
                           .price_ = 202.0,
                           .quantity_ = 20.0});
    engine.handle_book_update(
        asset2, BookUpdate{.timestamp_ = 30,
                           .localTimestamp_ = 40,
                           .update_type_ = UpdateType::Incremental,
                           .side_ = BookSide::Bid,
                           .price_ = 198.0,
                           .quantity_ = 20.0});

    SECTION("Submit market buy order for asset 1") {
        auto buy_order =
            std::make_shared<Order>(Order{.timestamp_ = 100,
                                          .orderId_ = 1,
                                          .side_ = BookSide::Bid,
                                          .price_ = 0.0,
                                          .quantity_ = 5.0,
                                          .filled_quantity_ = 0.0,
                                          .tif_ = TimeInForce::GTX,
                                          .orderType_ = OrderType::LIMIT,
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
            std::make_shared<Order>(Order{.timestamp_ = 200,
                                          .orderId_ = 2,
                                          .side_ = BookSide::Ask,
                                          .price_ = 0.0,
                                          .quantity_ = 10.0,
                                          .filled_quantity_ = 0.0,
                                          .tif_ = TimeInForce::GTX,
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
            std::make_shared<Order>(Order{.timestamp_ = 300,
                                          .orderId_ = 3,
                                          .side_ = BookSide::Bid,
                                          .price_ = 100.0,
                                          .quantity_ = 3.0,
                                          .filled_quantity_ = 0.0,
                                          .tif_ = TimeInForce::GTX,
                                          .orderType_ = OrderType::LIMIT,
                                          .queueEst_ = 0.0});

        auto order2 =
            std::make_shared<Order>(Order{.timestamp_ = 301,
                                          .orderId_ = 4,
                                          .side_ = BookSide::Bid,
                                          .price_ = 200.0,
                                          .quantity_ = 4.0,
                                          .filled_quantity_ = 0.0,
                                          .tif_ = TimeInForce::GTX,
                                          .orderType_ = OrderType::LIMIT,
                                          .queueEst_ = 0.0});

        engine.place_gtx_order(asset1, order1);
        engine.place_gtx_order(asset2, order2);

        const auto &all_orders1 = engine.orders(asset1);
        const auto &all_orders2 = engine.orders(asset2);

        REQUIRE(all_orders1.size() == 1);
        REQUIRE(all_orders2.size() == 1);
        REQUIRE(all_orders1[0].orderId_ != all_orders2[0].orderId_);
    }
}

TEST_CASE("[ExecutionEngine] - executes market buy and sell orders",
          "[execution-engine][market]") {
    ExecutionEngine engine;

    engine.handle_book_update(
        0, BookUpdate{.timestamp_ = 0,
                      .localTimestamp_ = 10,
                      .update_type_ = UpdateType::Incremental,
                      .side_ = BookSide::Ask,
                      .price_ = 101.0,
                      .quantity_ = 2.0});
    engine.handle_book_update(
        0, BookUpdate{.timestamp_ = 10,
                      .localTimestamp_ = 20,
                      .update_type_ = UpdateType::Incremental,
                      .side_ = BookSide::Ask,
                      .price_ = 102.0,
                      .quantity_ = 3.0});
    engine.handle_book_update(
        0, BookUpdate{.timestamp_ = 20,
                      .localTimestamp_ = 30,
                      .update_type_ = UpdateType::Incremental,
                      .side_ = BookSide::Bid,
                      .price_ = 100.0,
                      .quantity_ = 1.5});
    engine.handle_book_update(
        0, BookUpdate{.timestamp_ = 30,
                      .localTimestamp_ = 40,
                      .update_type_ = UpdateType::Incremental,
                      .side_ = BookSide::Bid,
                      .price_ = 99.0,
                      .quantity_ = 1.0});

    SECTION("Market buy order fills against ask levels") {
        auto buy_order =
            std::make_shared<Order>(Order{.timestamp_ = 50,
                                          .orderId_ = 1,
                                          .price_ = 0.0,
                                          .quantity_ = 4.0,
                                          .filled_quantity_ = 0.0,
                                          .tif_ = TimeInForce::GTC,
                                          .orderType_ = OrderType::MARKET});

        engine.execute_market_order(0, TradeSide::Buy, buy_order);

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
            std::make_shared<Order>(Order{.timestamp_ = 50,
                                          .orderId_ = 2,
                                          .price_ = 0.0,
                                          .quantity_ = 2.0,
                                          .filled_quantity_ = 0.0,
                                          .tif_ = TimeInForce::GTC,
                                          .orderType_ = OrderType::MARKET});

        engine.execute_market_order(0, TradeSide::Sell, sell_order);

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
    ExecutionEngine engine;

    engine.handle_book_update(
        0, BookUpdate{.timestamp_ = 0,
                      .localTimestamp_ = 10,
                      .update_type_ = UpdateType::Incremental,
                      .side_ = BookSide::Ask,
                      .price_ = 101.0,
                      .quantity_ = 2.0});
    engine.handle_book_update(
        0, BookUpdate{.timestamp_ = 10,
                      .localTimestamp_ = 20,
                      .update_type_ = UpdateType::Incremental,
                      .side_ = BookSide::Ask,
                      .price_ = 102.0,
                      .quantity_ = 3.0});
    engine.handle_book_update(
        0, BookUpdate{.timestamp_ = 20,
                      .localTimestamp_ = 30,
                      .update_type_ = UpdateType::Incremental,
                      .side_ = BookSide::Bid,
                      .price_ = 100.0,
                      .quantity_ = 1.5});
    engine.handle_book_update(
        0, BookUpdate{.timestamp_ = 30,
                      .localTimestamp_ = 40,
                      .update_type_ = UpdateType::Incremental,
                      .side_ = BookSide::Bid,
                      .price_ = 99.0,
                      .quantity_ = 1.0});

    SECTION("Limit buy fill-or-kill order fills against ask levels") {
        auto buy_order =
            std::make_shared<Order>(Order{.timestamp_ = 50,
                                          .orderId_ = 1,
                                          .price_ = 101.5,
                                          .quantity_ = 3.0,
                                          .filled_quantity_ = 0.0,
                                          .tif_ = TimeInForce::FOK,
                                          .orderType_ = OrderType::LIMIT});
        REQUIRE(engine.execute_fok_order(0, TradeSide::Buy, buy_order) ==
                false);

        const auto &fills = engine.fills();
        REQUIRE(buy_order->filled_quantity_ == 0.0);
        REQUIRE(fills.size() == 0);
    }
    SECTION("Limit sell fill-or-kill order fills against bid levels") {
        auto sell_order =
            std::make_shared<Order>(Order{.timestamp_ = 50,
                                          .orderId_ = 2,
                                          .price_ = 99.0,
                                          .quantity_ = 2.0,
                                          .filled_quantity_ = 0.0,
                                          .tif_ = TimeInForce::FOK,
                                          .orderType_ = OrderType::LIMIT});
        REQUIRE(engine.execute_fok_order(0, TradeSide::Sell, sell_order) ==
                true);

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
    ExecutionEngine engine;

    engine.handle_book_update(
        0, BookUpdate{.timestamp_ = 0,
                      .localTimestamp_ = 10,
                      .update_type_ = UpdateType::Incremental,
                      .side_ = BookSide::Ask,
                      .price_ = 101.0,
                      .quantity_ = 2.0});
    engine.handle_book_update(
        0, BookUpdate{.timestamp_ = 10,
                      .localTimestamp_ = 20,
                      .update_type_ = UpdateType::Incremental,
                      .side_ = BookSide::Ask,
                      .price_ = 102.0,
                      .quantity_ = 3.0});
    engine.handle_book_update(
        0, BookUpdate{.timestamp_ = 20,
                      .localTimestamp_ = 30,
                      .update_type_ = UpdateType::Incremental,
                      .side_ = BookSide::Bid,
                      .price_ = 100.0,
                      .quantity_ = 1.5});
    engine.handle_book_update(
        0, BookUpdate{.timestamp_ = 30,
                      .localTimestamp_ = 40,
                      .update_type_ = UpdateType::Incremental,
                      .side_ = BookSide::Bid,
                      .price_ = 99.0,
                      .quantity_ = 1.0});

    SECTION("Limit buy immediate-or-cancel order fills against ask levels") {
        auto buy_order =
            std::make_shared<Order>(Order{.timestamp_ = 50,
                                          .orderId_ = 1,
                                          .price_ = 101.5,
                                          .quantity_ = 3.0,
                                          .filled_quantity_ = 0.0,
                                          .tif_ = TimeInForce::IOC,
                                          .orderType_ = OrderType::LIMIT});
        REQUIRE(engine.execute_ioc_order(0, TradeSide::Buy, buy_order) == true);

        const auto &fills = engine.fills();
        REQUIRE(buy_order->filled_quantity_ == 2.0);
        REQUIRE(fills.size() == 1);
        REQUIRE(fills[0].price_ == 101.0);
        REQUIRE(fills[0].quantity_ == 2.0);
    }
    SECTION("Limit sell immediate-or-cancel order fills against bid levels") {
        auto sell_order =
            std::make_shared<Order>(Order{.timestamp_ = 50,
                                          .orderId_ = 2,
                                          .price_ = 99.0,
                                          .quantity_ = 2.0,
                                          .filled_quantity_ = 0.0,
                                          .tif_ = TimeInForce::IOC,
                                          .orderType_ = OrderType::LIMIT});
        REQUIRE(engine.execute_ioc_order(0, TradeSide::Sell, sell_order) ==
                true);

        const auto &fills = engine.fills();
        REQUIRE(sell_order->filled_quantity_ == 2.0);
        REQUIRE(fills.size() == 2);
        REQUIRE(fills[0].price_ == 100.0);
        REQUIRE(fills[0].quantity_ == 1.5);
        REQUIRE(fills[1].price_ == 99.0);
        REQUIRE(fills[1].quantity_ == 0.5);
    }
}

TEST_CASE("[ExecutionEngine] - places limit GTX orders ",
          "[execution-engine][GTX]") {
    ExecutionEngine engine;

    engine.handle_book_update(
        0, BookUpdate{.timestamp_ = 0,
                      .localTimestamp_ = 10,
                      .update_type_ = UpdateType::Incremental,
                      .side_ = BookSide::Ask,
                      .price_ = 101.0,
                      .quantity_ = 2.0});
    engine.handle_book_update(
        0, BookUpdate{.timestamp_ = 10,
                      .localTimestamp_ = 20,
                      .update_type_ = UpdateType::Incremental,
                      .side_ = BookSide::Ask,
                      .price_ = 102.0,
                      .quantity_ = 3.0});
    engine.handle_book_update(
        0, BookUpdate{.timestamp_ = 20,
                      .localTimestamp_ = 30,
                      .update_type_ = UpdateType::Incremental,
                      .side_ = BookSide::Bid,
                      .price_ = 100.0,
                      .quantity_ = 1.5});
    engine.handle_book_update(
        0, BookUpdate{.timestamp_ = 30,
                      .localTimestamp_ = 40,
                      .update_type_ = UpdateType::Incremental,
                      .side_ = BookSide::Bid,
                      .price_ = 99.0,
                      .quantity_ = 1.0});

    SECTION("Places limit GTX buy order against bid levels") {
        auto buy_order_1 =
            std::make_shared<Order>(Order{.timestamp_ = 50,
                                          .orderId_ = 1,
                                          .side_ = BookSide::Bid,
                                          .price_ = 101.5,
                                          .quantity_ = 3.0,
                                          .filled_quantity_ = 0.0,
                                          .tif_ = TimeInForce::GTX,
                                          .orderType_ = OrderType::LIMIT,
                                          .queueEst_ = 0.0});
        REQUIRE(engine.place_gtx_order(0, buy_order_1) == false);
        const auto &fills_1 = engine.fills();
        REQUIRE(fills_1.size() == 0);

        auto buy_order_2 =
            std::make_shared<Order>(Order{.timestamp_ = 50,
                                          .orderId_ = 1,
                                          .side_ = BookSide::Bid,
                                          .price_ = 98.0,
                                          .quantity_ = 3.0,
                                          .filled_quantity_ = 0.0,
                                          .tif_ = TimeInForce::GTX,
                                          .orderType_ = OrderType::LIMIT,
                                          .queueEst_ = 0.0});
        REQUIRE(engine.place_gtx_order(0, buy_order_2) == true);
        const auto &fills_2 = engine.fills();
        REQUIRE(fills_2.size() == 0);
    }
    SECTION("Places limit GTX sell order against ask levels") {
        auto sell_order_1 =
            std::make_shared<Order>(Order{.timestamp_ = 50,
                                          .orderId_ = 2,
                                          .side_ = BookSide::Ask,
                                          .price_ = 99.0,
                                          .quantity_ = 2.0,
                                          .filled_quantity_ = 0.0,
                                          .tif_ = TimeInForce::GTX,
                                          .orderType_ = OrderType::LIMIT});
        REQUIRE(engine.place_gtx_order(0, sell_order_1) == false);
        const auto &fills_1 = engine.fills();
        REQUIRE(fills_1.size() == 0);

        auto sell_order_2 =
            std::make_shared<Order>(Order{.timestamp_ = 50,
                                          .orderId_ = 2,
                                          .side_ = BookSide::Ask,
                                          .price_ = 102.0,
                                          .quantity_ = 2.0,
                                          .filled_quantity_ = 0.0,
                                          .tif_ = TimeInForce::GTX,
                                          .orderType_ = OrderType::LIMIT});
        REQUIRE(engine.place_gtx_order(0, sell_order_2) == true);
        REQUIRE(sell_order_2->queueEst_ == 3.0);
        const auto &fills_2 = engine.fills();
        REQUIRE(fills_2.size() == 0);
    }
}

TEST_CASE("[ExecutionEngine] - queue estimation", "[execution-engine][queue]") {
    ExecutionEngine engine;

    engine.handle_book_update(
        0, BookUpdate{.timestamp_ = 10,
                      .localTimestamp_ = 20,
                      .update_type_ = UpdateType::Incremental,
                      .side_ = BookSide::Ask,
                      .price_ = 102.0,
                      .quantity_ = 3.0});
    engine.handle_book_update(
        0, BookUpdate{.timestamp_ = 30,
                      .localTimestamp_ = 40,
                      .update_type_ = UpdateType::Incremental,
                      .side_ = BookSide::Bid,
                      .price_ = 99.0,
                      .quantity_ = 1.0});

    auto buy_order_1 =
        std::make_shared<Order>(Order{.timestamp_ = 50,
                                      .orderId_ = 1,
                                      .side_ = BookSide::Bid,
                                      .price_ = 99.0,
                                      .quantity_ = 3.0,
                                      .filled_quantity_ = 0.0,
                                      .tif_ = TimeInForce::GTX,
                                      .orderType_ = OrderType::LIMIT,
                                      .queueEst_ = 0.0});
    auto sell_order_1 =
        std::make_shared<Order>(Order{.timestamp_ = 50,
                                      .orderId_ = 2,
                                      .side_ = BookSide::Ask,
                                      .price_ = 102.0,
                                      .quantity_ = 1.0,
                                      .filled_quantity_ = 0.0,
                                      .tif_ = TimeInForce::GTX,
                                      .orderType_ = OrderType::LIMIT,
                                      .queueEst_ = 0.0});

    SECTION("Initial queue estimation") {
        // placed at the end of existing depth at price level
        engine.place_gtx_order(0, buy_order_1);
        engine.place_gtx_order(0, sell_order_1);
        REQUIRE(buy_order_1->queueEst_ == 1.0);
        REQUIRE(sell_order_1->queueEst_ == 3.0);
    }
    SECTION("Queue estimation updates") {
        // f(x) = ln(1+x)
        engine.place_gtx_order(0, buy_order_1);
        engine.place_gtx_order(0, sell_order_1);

        engine.handle_book_update(
            0, BookUpdate{.timestamp_ = 60,
                          .localTimestamp_ = 70,
                          .update_type_ = UpdateType::Incremental,
                          .side_ = BookSide::Bid,
                          .price_ = 99.0,
                          .quantity_ = 0.2});

        engine.handle_book_update(
            0, BookUpdate{.timestamp_ = 70,
                          .localTimestamp_ = 80,
                          .update_type_ = UpdateType::Incremental,
                          .side_ = BookSide::Ask,
                          .price_ = 102.0,
                          .quantity_ = 5.0});
        // buy order at bottom of queue, queue est. should decrease by delta
        REQUIRE(buy_order_1->queueEst_ == Catch::Approx(0.2).margin(1e-8));
        // sell order position remains unchanged
        REQUIRE(sell_order_1->queueEst_ == Catch::Approx(3.0).margin(1e-8));

        engine.handle_book_update(
            0, BookUpdate{.timestamp_ = 80,
                          .localTimestamp_ = 90,
                          .update_type_ = UpdateType::Incremental,
                          .side_ = BookSide::Ask,
                          .price_ = 102.0,
                          .quantity_ = 1.0});
        REQUIRE(sell_order_1->queueEst_ ==
                Catch::Approx(1.0 / 3.0).margin(1e-8));
        engine.handle_book_update(
            0, BookUpdate{.timestamp_ = 90,
                          .localTimestamp_ = 100,
                          .update_type_ = UpdateType::Incremental,
                          .side_ = BookSide::Ask,
                          .price_ = 102.0,
                          .quantity_ = 0.5});
        REQUIRE(sell_order_1->queueEst_ == 0.0);
    }
}

TEST_CASE("[ExecutionEngine] - processes trades", "[execution-engine][trade]") {
    SECTION("Full execution of bid order when sufficient trade size") {
        ExecutionEngine engine;

        auto buy_order =
            std::make_shared<Order>(Order{.timestamp_ = 10,
                                          .orderId_ = 1,
                                          .side_ = BookSide::Bid,
                                          .price_ = 100.0,
                                          .quantity_ = 1.0,
                                          .filled_quantity_ = 0.0,
                                          .tif_ = TimeInForce::GTX,
                                          .orderType_ = OrderType::LIMIT,
                                          .queueEst_ = 0.0});
        engine.place_gtx_order(0, buy_order);
        REQUIRE(buy_order->queueEst_ == 0.0);

        engine.handle_trade(0, Trade{.timestamp_ = 20,
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
            std::make_shared<Order>(Order{.timestamp_ = 30,
                                          .orderId_ = 2,
                                          .side_ = BookSide::Ask,
                                          .price_ = 102.0,
                                          .quantity_ = 1.0,
                                          .filled_quantity_ = 0.0,
                                          .tif_ = TimeInForce::GTX,
                                          .orderType_ = OrderType::LIMIT,
                                          .queueEst_ = 0.0});
        engine.place_gtx_order(0, sell_order);

        engine.handle_trade(0, Trade{.timestamp_ = 40,
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
        ExecutionEngine engine;

        auto buy_order =
            std::make_shared<Order>(Order{.timestamp_ = 10,
                                          .orderId_ = 1,
                                          .side_ = BookSide::Bid,
                                          .price_ = 101.0,
                                          .quantity_ = 1.5,
                                          .filled_quantity_ = 0.0,
                                          .tif_ = TimeInForce::GTX,
                                          .orderType_ = OrderType::LIMIT,
                                          .queueEst_ = 0.0});
        engine.place_gtx_order(0, buy_order);
        engine.handle_trade(0, Trade{.timestamp_ = 20,
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
            std::make_shared<Order>(Order{.timestamp_ = 30,
                                          .orderId_ = 1,
                                          .side_ = BookSide::Ask,
                                          .price_ = 103.0,
                                          .quantity_ = 3.5,
                                          .filled_quantity_ = 0.0,
                                          .tif_ = TimeInForce::GTX,
                                          .orderType_ = OrderType::LIMIT,
                                          .queueEst_ = 0.0});
        engine.place_gtx_order(0, sell_order);
        engine.handle_trade(0, Trade{.timestamp_ = 40,
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
        ExecutionEngine engine;

        auto buy_order =
            std::make_shared<Order>(Order{.timestamp_ = 30,
                                          .orderId_ = 2,
                                          .side_ = BookSide::Bid,
                                          .price_ = 101.0,
                                          .quantity_ = 2.0,
                                          .filled_quantity_ = 0.0,
                                          .tif_ = TimeInForce::GTX,
                                          .orderType_ = OrderType::LIMIT,
                                          .queueEst_ = 0.0});
        engine.place_gtx_order(0, buy_order);
        engine.handle_trade(0, Trade{.timestamp_ = 20,
                                     .local_timestamp_ = 21,
                                     .side_ = TradeSide::Sell,
                                     .price_ = 101.0,
                                     .quantity_ = 1.0,
                                     .orderId_ = 1234});

        REQUIRE(buy_order->filled_quantity_ == 0.0);
        REQUIRE(engine.fills().empty());

        auto sell_order =
            std::make_shared<Order>(Order{.timestamp_ = 60,
                                          .orderId_ = 2,
                                          .side_ = BookSide::Ask,
                                          .price_ = 101.0,
                                          .quantity_ = 2.0,
                                          .filled_quantity_ = 0.0,
                                          .tif_ = TimeInForce::GTX,
                                          .orderType_ = OrderType::LIMIT,
                                          .queueEst_ = 0.0});
        engine.place_gtx_order(0, sell_order);
        engine.handle_trade(0, Trade{.timestamp_ = 40,
                                     .local_timestamp_ = 21,
                                     .side_ = TradeSide::Buy,
                                     .price_ = 101.0,
                                     .quantity_ = 1.0,
                                     .orderId_ = 1234});

        REQUIRE(buy_order->filled_quantity_ == 0.0);
        REQUIRE(engine.fills().empty());
    }
}
