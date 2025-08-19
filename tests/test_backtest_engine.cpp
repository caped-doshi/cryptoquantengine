/*
 * File: tests/test_backtest_engine.cpp
 * Description: Unit tests for backtest_engine.cpp
 * Author: Arvind Rathnashyam
 * Date: 2025-07-02
 * License: Proprietary
 */

#include <catch2/catch_approx.hpp>
#include <catch2/catch_test_macros.hpp>
#include <filesystem>
#include <fstream>
#include <memory>

#include "core/execution_engine/execution_engine.h"
#include "core/market_data/book_update.h"
#include "core/orderbook/orderbook.h"
#include "core/trading/backtest_engine.h"
#include "core/types/book_side.h"
#include "core/types/order_type.h"
#include "core/types/time_in_force.h"
#include "core/types/update_type.h"
#include "core/types/usings.h"
#include "utils/logger/log_level.h"
#include "utils/logger/logger.h"
#include "utils/math/math_utils.h"

namespace TestHelpers {
void create_trade_csv(const std::string &filename) {
    std::ofstream f(filename);
    f << "timestamp,local_timestamp,id,side,price,amount\n"
      << "10000,11000,1,buy,50000.0,1.0\n"
      << "11000,12000,2,buy,50000.5,1.0\n"
      << "13000,14000,3,sell,50001.0,0.5\n"
      << "14000,15000,4,sell,50001.0,1.5\n";
}

void create_book_update_csv(const std::string &filename) {
    std::ofstream f(filename);
    f << "timestamp,local_timestamp,is_snapshot,side,price,amount\n"
      << "1000,2000,false,ask,50001.0,1.5\n"
      << "20000,21000,false,bid,50000.0,2.0\n"
      << "30000,31000,false,bid,50000.5,2.0\n"
      << "40000,41000,false,ask,50001.0,1.5\n"
      << "50000,51000,false,ask,50001.0,2.5\n";
}
} // namespace TestHelpers

TEST_CASE("[BacktestEngine] - initializes correctly with valid input",
          "[backtest][init]") {
    const std::string book_file = "test_book.csv";
    const std::string trade_file = "test_trade.csv";
    TestHelpers::create_book_update_csv(book_file);
    TestHelpers::create_trade_csv(trade_file);

    // Config for a single asset
    int asset_id = 1;
    std::unordered_map<int, AssetConfig> asset_configs = {
        {asset_id, AssetConfig{.book_update_file_ = book_file,
                               .trade_file_ = trade_file,
                               .tick_size_ = 0.001,
                               .lot_size_ = 0.00001,
                               .contract_multiplier_ = 1.0,
                               .is_inverse_ = false,
                               .maker_fee_ = 0.0,
                               .taker_fee_ = 0.00045}}};
    auto backtest_engine_config =
        BacktestEngineConfig{.initial_cash_ = 1000.0,
                             .order_entry_latency_us_ = 1000,
                             .order_response_latency_us_ = 1000,
                             .market_feed_latency_us_ = 1000};
    // Logger
    auto logger = std::make_shared<Logger>("test_backtest_engine_init.log",
                                           LogLevel::Debug);

    SECTION("Construct BacktestEngine with valid file inputs") {
        bool threw = false;
        try {
            BacktestEngine engine(asset_configs, backtest_engine_config,
                                  logger);
        } catch (...) {
            threw = true;
        }
        REQUIRE(!threw);
    }

    // Cleanup
    std::filesystem::remove(book_file);
    std::filesystem::remove(trade_file);
    logger->flush();
}

TEST_CASE("[BacktestEngine] - rejects invalid orders",
          "[backtest-engine][invalid]") {
    const std::string book_file = "test_book.csv";
    const std::string trade_file = "test_trade.csv";
    TestHelpers::create_book_update_csv(book_file);
    TestHelpers::create_trade_csv(trade_file);

    int asset_id = 1;
    Depth depth;

    std::unordered_map<int, AssetConfig> asset_configs = {
        {asset_id, AssetConfig{.book_update_file_ = book_file,
                               .trade_file_ = trade_file,
                               .tick_size_ = 0.001,
                               .lot_size_ = 0.00001,
                               .contract_multiplier_ = 1.0,
                               .is_inverse_ = false,
                               .maker_fee_ = 0.0,
                               .taker_fee_ = 0.00045}}};
    auto backtest_engine_config =
        BacktestEngineConfig{.initial_cash_ = 1000.0,
                             .order_entry_latency_us_ = 1000,
                             .order_response_latency_us_ = 1000,
                             .market_feed_latency_us_ = 1000};
    auto logger = std::make_shared<Logger>("test_backtest_engine_invalid.log",
                                           LogLevel::Debug);
    BacktestEngine hbt(asset_configs, backtest_engine_config, logger);

    // Invalid price (0 or negative)
    REQUIRE_THROWS_AS(hbt.submit_buy_order(asset_id, 0.0, 1.0, TimeInForce::GTC,
                                           OrderType::LIMIT),
                      std::invalid_argument);
    REQUIRE_THROWS_AS(hbt.submit_sell_order(asset_id, -1.0, 1.0,
                                            TimeInForce::GTC, OrderType::LIMIT),
                      std::invalid_argument);
    // Invalid quantity
    REQUIRE_THROWS_AS(hbt.submit_buy_order(asset_id, 50000.0, 0.0,
                                           TimeInForce::GTC, OrderType::LIMIT),
                      std::invalid_argument);
    // Cleanup
    std::filesystem::remove(book_file);
    std::filesystem::remove(trade_file);
    logger->flush();
}

TEST_CASE("[BacktestEngine] - elapse", "[backtest-engine][elapse]") {
    const std::string book_file = "test_book.csv";
    const std::string trade_file = "test_trade.csv";
    TestHelpers::create_book_update_csv(book_file);
    TestHelpers::create_trade_csv(trade_file);

    int asset_id = 1;
    double tick_size = 0.001;
    double lot_size = 0.00001;
    Depth depth;

    std::unordered_map<int, AssetConfig> asset_configs = {
        {asset_id, AssetConfig{.book_update_file_ = book_file,
                               .trade_file_ = trade_file,
                               .tick_size_ = tick_size,
                               .lot_size_ = lot_size,
                               .contract_multiplier_ = 1.0,
                               .is_inverse_ = false,
                               .maker_fee_ = 0.0,
                               .taker_fee_ = 0.00045}}};
    auto backtest_engine_config =
        BacktestEngineConfig{.initial_cash_ = 0.0,
                             .order_entry_latency_us_ = 1000,
                             .order_response_latency_us_ = 1000,
                             .market_feed_latency_us_ = 1000};

    SECTION("Current timestamp elapses correctly") {
        auto logger = std::make_shared<Logger>(
            "test_backtest_engine_elapse_correct.log", LogLevel::Debug);
        BacktestEngine hbt(asset_configs, backtest_engine_config, logger);

        REQUIRE(hbt.elapse(100) == true);
        REQUIRE(hbt.current_time() == 100);
        REQUIRE(hbt.elapse(100) == true);
        REQUIRE(hbt.current_time() == 200);
        // Cleanup
        logger->flush();
    }
    SECTION("local orderbook updated with latency") {
        auto logger = std::make_shared<Logger>(
            "test_backtest_engine_elapse_latency.log", LogLevel::Debug);
        BacktestEngine hbt(asset_configs, backtest_engine_config, logger);

        REQUIRE(hbt.elapse(50000) == true);
        REQUIRE(hbt.current_time() == 50000);

        depth = hbt.depth(asset_id);
        REQUIRE(depth.best_ask_ == price_to_ticks(50001.0, tick_size));
        REQUIRE(depth.best_bid_ == price_to_ticks(50000.5, tick_size));
        REQUIRE(depth.ask_qty_ == 1.5);
        REQUIRE(depth.bid_qty_ == 2.0);
        REQUIRE(depth.bid_depth_[price_to_ticks(50000.5, tick_size)] == 2.0);
        REQUIRE(depth.ask_depth_[price_to_ticks(50001.0, tick_size)] == 1.5);

        REQUIRE(hbt.elapse(2000) == true);
        REQUIRE(hbt.current_time() == 52000);

        depth = hbt.depth(asset_id);
        REQUIRE(depth.ask_qty_ == 2.5);
        // Cleanup
        logger->flush();
    }
    SECTION("Market order executed in correct schedule") {
        auto logger = std::make_shared<Logger>(
            "test_backtest_engine_elapse_market_schedule.log", LogLevel::Debug);
        BacktestEngine hbt(asset_configs, backtest_engine_config, logger);

        REQUIRE(hbt.elapse(29500));
        REQUIRE(hbt.current_time() == 29500);

        depth = hbt.depth(asset_id);
        REQUIRE(depth.best_bid_ == price_to_ticks(50000.0, tick_size));
        REQUIRE(depth.bid_qty_ == 2.0);
        // Submit a buy order that will be delayed and scheduled
        OrderId order_id = hbt.submit_sell_order(
            asset_id, 0.0, 1.0, TimeInForce::GTC, OrderType::MARKET);

        // Initially, no fills processed yet
        REQUIRE(hbt.position(asset_id) == 0.0);

        REQUIRE(
            hbt.elapse(5000)); // Enough to trigger the trade and delayed action
        REQUIRE(hbt.current_time() == 34500);

        // Now the order should be filled
        REQUIRE(hbt.position(asset_id) == -1.0);
        REQUIRE(hbt.cash() ==
                Catch::Approx(50000.5 * (1 - 0.00045)).margin(1e-8));
        // Cleanup
        logger->flush();
    }
    SECTION("Limit order executed in correct schedule") {
        auto logger = std::make_shared<Logger>(
            "test_backtest_engine_elapse_limit_schedule.log", LogLevel::Debug);
        BacktestEngine hbt(asset_configs, backtest_engine_config, logger);

        REQUIRE(hbt.elapse(5000));
        REQUIRE(hbt.current_time() == 5000);

        depth = hbt.depth(asset_id);
        REQUIRE(depth.best_ask_ == price_to_ticks(50001.0, tick_size));

        OrderId order_id = hbt.submit_sell_order(
            asset_id, 50000.5, 1.0, TimeInForce::GTC, OrderType::LIMIT);

        REQUIRE(hbt.elapse(6500));
        REQUIRE(hbt.current_time() == 11500);
        REQUIRE(hbt.position(asset_id) == 0.0);

        REQUIRE(hbt.elapse(5000));
        REQUIRE(hbt.current_time() == 16500);
        REQUIRE(hbt.position(asset_id) == -1.0);
        // Cleanup
        logger->flush();
    }

    SECTION("Complex multi-limit order execution with partial fills and "
            "cancellations") {
        auto logger = std::make_shared<Logger>(
            "test_backtest_engine_elapse_complex.log", LogLevel::Debug);
        BacktestEngine hbt(asset_configs, backtest_engine_config, logger);

        REQUIRE(hbt.elapse(5000));
        REQUIRE(hbt.current_time() == 5000);

        OrderId order1 = hbt.submit_sell_order(
            asset_id, 50000.5, 1.0, TimeInForce::GTC, OrderType::LIMIT);
        OrderId order2 = hbt.submit_sell_order(
            asset_id, 50001.0, 2.0, TimeInForce::GTC, OrderType::LIMIT);

        // Verify initial state
        REQUIRE(hbt.elapse(3000));
        REQUIRE(hbt.current_time() == 8000);
        REQUIRE(hbt.position(asset_id) == 0.0);
        REQUIRE(hbt.orders(asset_id).size() == 2);

        // Partial fill: Best bid moves to 50000.5, fills 1.0 of order1
        REQUIRE(hbt.elapse(10000)); // Advance to t=15000
        REQUIRE(hbt.current_time() == 18000);
        REQUIRE(hbt.position(asset_id) == -1.0); // Partially filled
        hbt.clear_inactive_orders();
        REQUIRE(hbt.orders(asset_id).size() ==
                1); // order1 fully filled and removed

        // Cancel order2 before it's filled
        hbt.cancel_order(asset_id, order2);
        REQUIRE(hbt.elapse(2100));
        REQUIRE(hbt.current_time() == 20100);
        REQUIRE(hbt.orders(asset_id).size() == 0);

        // Verify cash balance after fills (assume maker fee 0.0000)
        double expected_cash = 50000.5;
        REQUIRE(hbt.cash() == Catch::Approx(expected_cash).margin(1e-8));
        // Cleanup
        logger->flush();
    }
    SECTION("Handles partial fills correctly") {
        auto logger = std::make_shared<Logger>(
            "test_backtest_engine_elapse_partial_fills.log", LogLevel::Debug);
        BacktestEngine hbt(asset_configs, backtest_engine_config, logger);

        hbt.elapse(5000);
        // Submit order larger than available liquidity
        OrderId big_order = hbt.submit_sell_order(
            asset_id, 50000.5, 5.0, TimeInForce::GTC, OrderType::LIMIT);

        hbt.elapse(7001);
        // Should partially fill available quantity (2.0)
        REQUIRE(hbt.position(asset_id) == -1.0);
        REQUIRE(hbt.orders(asset_id).size() == 1); // Remainder still active
        REQUIRE(hbt.orders(asset_id)[0].filled_quantity_ == 1.0);
        // Cleanup
        logger->flush();
    }
    std::filesystem::remove(book_file);
    std::filesystem::remove(trade_file);
}