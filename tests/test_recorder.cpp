/*
 * File: tests/test_recorder.cpp
 * Description: Unit tests for the Recorder class.
 * Author: Arvind Rathnashyam
 * Date: 2025-07-25
 * License: Proprietary
 */

#include <catch2/catch_approx.hpp>
#include <catch2/catch_test_macros.hpp>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <memory>

#include "../hftengine/core/recorder/recorder.h"
#include "../hftengine/utils/stat/stat_utils.h"

TEST_CASE("[Recorder] - recorder interval returns", "[recorder][initial]") {
    Recorder recorder(1'000'000);
    SECTION("Empty recorder returns empty vectors") {
        REQUIRE(recorder.interval_returns().empty());
        REQUIRE_THROWS_AS(recorder.sharpe(), std::runtime_error);
        REQUIRE_THROWS_AS(recorder.sortino(), std::runtime_error);
    }
    SECTION("Single record returns empty returns vector") {
        recorder.record(0, 100.0);
        REQUIRE(recorder.interval_returns().empty());
    }
}

TEST_CASE("[Recorder] - interval returns calculation", "[recorder][returns]") {
    Recorder recorder(1'000'000);
    SECTION("Process multiple returns") {
        recorder.record(0, 100.0);
        recorder.record(500'000, 105.0);
        recorder.record(1'000'000, 110.0);
        recorder.record(1'500'000, 99.0);
        recorder.record(2'500'000, 108.9);

        auto returns = recorder.interval_returns();
        REQUIRE(returns.size() == 3);
        REQUIRE(returns[0] == Catch::Approx(0.1).margin(1e-8));
        REQUIRE(returns[1] == Catch::Approx(-0.1).margin(1e-8));
        REQUIRE(returns[2] == Catch::Approx(0.1).margin(1e-8));
    }
}

TEST_CASE("[Recorder] - Risk-adjusted metrics edge cases", "[recorder][edge]") {
    Recorder recorder(1'000'000); // 1 second interval

    SECTION("Sharpe ratio calculation") {
        recorder.record(0, 100.0);
        recorder.record(500'000, 101.0);
        recorder.record(1'500'000, 102.01);

        auto returns = recorder.interval_returns();
        REQUIRE(returns.size() == 2);
        REQUIRE(returns[0] == Catch::Approx(0.01).margin(1e-8));
        REQUIRE(returns[1] == Catch::Approx(0.01).margin(1e-8));

        REQUIRE_THROWS_AS(recorder.sharpe(), std::runtime_error);
    }

    SECTION("Sortino ratio calculation") {
        // Volatile returns with some negative values
        recorder.record(0, 100.0);
        recorder.record(1'000'000, 110.0); // +10%
        recorder.record(2'000'000, 99.0);  // -10%
        recorder.record(3'000'000, 108.9); // +10%

        // Should calculate without throwing
        REQUIRE_THROWS_AS(recorder.sortino(), std::runtime_error);
    }

    SECTION("Edge cases for risk metrics") {
        recorder.record(0, 100.0);
        recorder.record(1'000'000, 100.0); // 0% return

        // Zero returns should cause division by zero in both metrics
        REQUIRE_THROWS_AS(recorder.sharpe(), std::runtime_error);
        REQUIRE_THROWS_AS(recorder.sortino(), std::runtime_error);
    }
}

TEST_CASE("[Recorder] - Risk adjusted metrics correctness",
          "[recorder][correctness]") {

    Recorder recorder(60'000'000);

    recorder.record(0, 100.0);
    recorder.record(50'000'000, 110.0);
    recorder.record(110'000'000, 121.0);
    recorder.record(170'000'000, 108.9);
    recorder.record(230'000'000, 87.12);
    recorder.record(290'000'000, 100.188);

    auto returns = recorder.interval_returns();
    REQUIRE(returns.size() == 5);

    REQUIRE(returns[0] == Catch::Approx(0.1).margin(1e-8));
    REQUIRE(returns[1] == Catch::Approx(0.1).margin(1e-8));
    REQUIRE(returns[2] == Catch::Approx(-0.1).margin(1e-8));
    REQUIRE(returns[3] == Catch::Approx(-0.2).margin(1e-8));
    REQUIRE(returns[4] == Catch::Approx(0.15).margin(1e-8));

    // Calculate expected values
    const double minutes_in_year = 365 * 24 * 60;
    const double annualization_factor = sqrt(minutes_in_year);

    std::vector<double> expected_returns = {0.1, 0.1, -0.1, -0.2, 0.15};
    std::vector<double> downside_returns = {-0.1, -0.2};

    SECTION("Sharpe ratio calculation") {
        double mean_return = mean(expected_returns);
        double stddev_return = stddev(expected_returns);
        double expected_sharpe =
            annualization_factor * mean_return / stddev_return;

        double actual_sharpe = recorder.sharpe();
        REQUIRE(actual_sharpe == Catch::Approx(expected_sharpe).margin(1e-6));
    }

    SECTION("Sortino ratio calculation") {
        double mean_return = mean(expected_returns);
        double downside_stddev = stddev(downside_returns);
        double expected_sortino =
            annualization_factor * mean_return / downside_stddev;

        double actual_sortino = recorder.sortino();
        REQUIRE(actual_sortino == Catch::Approx(expected_sortino).margin(1e-6));
    }

    SECTION("Max drawdown calculation") {
        double expected_max_dd = (121.0 - 87.12) / 121.0;
        double actual_max_dd = recorder.max_drawdown();

        REQUIRE(actual_max_dd == Catch::Approx(expected_max_dd).margin(1e-8));
    }
}

TEST_CASE("[Recorder] - Max drawdown edge cases", "[recorder][drawdown]") {
    SECTION("Empty recorder throws") {
        Recorder recorder(60'000'000);
        REQUIRE_THROWS_AS(recorder.max_drawdown(), std::runtime_error);
    }

    SECTION("Single record gives zero drawdown") {
        Recorder recorder(60'000'000);
        recorder.record(0, 100.0);
        REQUIRE(recorder.max_drawdown() == 0.0);
    }

    SECTION("All increasing values gives zero drawdown") {
        Recorder recorder(60'000'000);
        recorder.record(0, 100.0);
        recorder.record(60'000'000, 110.0);
        recorder.record(120'000'000, 121.0);
        REQUIRE(recorder.max_drawdown() == 0.0);
    }

    SECTION("All decreasing values gives max drawdown") {
        Recorder recorder(60'000'000);
        recorder.record(0, 100.0);
        recorder.record(60'000'000, 90.0);
        recorder.record(120'000'000, 81.0);
        REQUIRE(recorder.max_drawdown() ==
                Catch::Approx(0.19).margin(1e-6)); // (100-81)/100
    }
}

TEST_CASE("[Recorder] - record(BacktestEngine, int) with market orders and "
          "trade file",
          "[recorder][backtest][market_order]") {
    // Create a minimal trade file
    const std::string trade_file = "test_recorder_trade.csv";
    std::ofstream tf(trade_file);
    tf << "timestamp,local_timestamp,id,side,price,amount\n"
       << "3000,3500,1,buy,105.0,1.1\n"
       << "3500,4000,2,sell,95.0,1.0\n"
       << "7000,7500,3,buy,105.0,1.1\n"
       << "8000,8500,4,sell,95.0,1.0\n";
    tf.close();

    // Create a minimal book file (optional, but can help with price updates)
    const std::string book_file = "test_recorder_book.csv";
    std::ofstream bf(book_file);
    bf << "timestamp,local_timestamp,is_snapshot,side,price,amount\n"
       << "500,1000,false,bid,90.0,1.0\n"
       << "500,1000,false,ask,110.0,1.0\n";
    bf.close();

    // Asset config
    int asset_id = 1;
    double tick_size = 0.01, lot_size = 0.01;
    std::unordered_map<int, AssetConfig> asset_configs = {
        {asset_id, AssetConfig{.book_update_file_ = book_file,
                               .trade_file_ = trade_file,
                               .tick_size_ = tick_size,
                               .lot_size_ = lot_size,
                               .contract_multiplier_ = 1.0,
                               .is_inverse_ = false,
                               .maker_fee_ = 0.0,
                               .taker_fee_ = 0.0}}};

    BacktestEngine engine(asset_configs);
    engine.set_order_entry_latency(1000);
    engine.set_order_response_latency(1000);

    Recorder recorder(10'000);
    // Submit a tighter market
    engine.submit_buy_order(asset_id, 95.0, 3.0, TimeInForce::GTC,
                            OrderType::LIMIT);
    engine.submit_sell_order(asset_id, 105.0, 3.0, TimeInForce::GTC,
                             OrderType::LIMIT);
    engine.elapse(5'000); // Advance time to process the trades
    recorder.record(engine, asset_id);
    engine.elapse(5'000);
    recorder.record(engine, asset_id);

    auto returns = recorder.interval_returns();
    REQUIRE(!returns.empty());
    REQUIRE(returns[0] == Catch::Approx(1.0).margin(1e-8)); // 10.5 -> 21.0

    // Cleanup
    std::remove(trade_file.c_str());
    std::remove(book_file.c_str());
}