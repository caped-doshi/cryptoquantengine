/*
 * File: tests/test_config_reader.cpp
 * Description: Unit tests for hft/utils/config/config_reader.cpp.
 * Author: Arvind Rathnashyam
 * Date: 2025-06-25
 * License: Proprietary
 */

#include <catch2/catch_test_macros.hpp>
#include <filesystem>
#include <fstream>

#include "core/recorder/recorder_config.h"
#include "core/strategy/grid_trading_config.h"
#include "core/trading/asset_config.h"
#include "core/backtest_engine/backtest_config.h"
#include "core/backtest_engine/backtest_engine_config.h"
#include "utils/config/config_reader.h"

TEST_CASE("[ConfigReader] - get_asset_config returns correct AssetConfig",
          "[config][asset_config]") {
    using namespace utils::config;
    using namespace core::trading;

    const std::string config_file = "test_asset_config.tmp";
    {
        std::ofstream out(config_file);
        out << "book_update_file=test_book.csv\n"
            << "trade_file=test_trade.csv\n"
            << "tick_size=0.01\n"
            << "lot_size=0.001\n"
            << "contract_multiplier=1.0\n"
            << "is_inverse=0\n"
            << "maker_fee=0.0001\n"
            << "taker_fee=0.0002\n";
    }

    ConfigReader reader;
    const auto config = reader.get_asset_config(config_file);

    REQUIRE(config.book_update_file_ == "test_book.csv");
    REQUIRE(config.trade_file_ == "test_trade.csv");
    REQUIRE(config.tick_size_ == 0.01);
    REQUIRE(config.lot_size_ == 0.001);
    REQUIRE(config.contract_multiplier_ == 1.0);
    REQUIRE(config.is_inverse_ == false);
    REQUIRE(config.maker_fee_ == 0.0001);
    REQUIRE(config.taker_fee_ == 0.0002);

    std::filesystem::remove(config_file);
}

TEST_CASE("[ConfigReader] - get_grid_trading_config returns correct "
          "GridTradingConfig",
          "[config][grid_trading_config]") {
    using namespace utils::config;
    const std::string config_file = "test_grid_trading_config.tmp";
    {
        std::ofstream out(config_file);
        out << "tick_size=0.01\n"
            << "lot_size=0.00001\n"
            << "grid_num=3\n"
            << "grid_interval=10\n"
            << "half_spread=20\n"
            << "position_limit=10.0\n"
            << "notional_order_qty=100.0\n";
    }

    ConfigReader reader;
    const auto config = reader.get_grid_trading_config(config_file);

    REQUIRE(config.tick_size_ == 0.01);
    REQUIRE(config.lot_size_ == 0.00001);
    REQUIRE(config.grid_num_ == 3);
    REQUIRE(config.grid_interval_ == 10);
    REQUIRE(config.half_spread_ == 20);
    REQUIRE(config.position_limit_ == 10.0);
    REQUIRE(config.notional_order_qty_ == 100.0);

    std::filesystem::remove(config_file);
}

TEST_CASE("[ConfigReader] - get_asset_config throws on missing keys",
          "[config][asset_config][missing]") {
    using namespace utils::config;

    const std::string config_file = "test_asset_config_missing.tmp";
    // Omit some required keys
    {
        std::ofstream out(config_file);
        out << "book_update_file=test_book.csv\n"
            << "trade_file=test_trade.csv\n"
            // Missing tick_size
            << "lot_size=0.001\n"
            << "contract_multiplier=1.0\n"
            << "is_inverse=0\n"
            << "maker_fee=0.0001\n"
            << "taker_fee=0.0002\n";
    }

    ConfigReader reader;
    REQUIRE_THROWS_AS(reader.get_asset_config(config_file), std::runtime_error);

    std::filesystem::remove(config_file);
}

TEST_CASE("[ConfigReader] - get_grid_trading_config throws on missing keys",
          "[config][grid_trading_config][missing]") {
    using namespace utils::config;

    const std::string config_file = "test_grid_trading_config_missing.tmp";
    // Omit some required keys
    {
        std::ofstream out(config_file);
        out << "tick_size=0.01\n"
            // Missing lot_size
            << "grid_num=3\n"
            << "grid_interval=10\n"
            << "half_spread=20\n"
            << "position_limit=10.0\n"
            << "notional_order_qty=100.0\n";
    }

    ConfigReader reader;
    REQUIRE_THROWS_AS(reader.get_grid_trading_config(config_file),
                      std::runtime_error);

    std::filesystem::remove(config_file);
}

TEST_CASE("[ConfigReader] - get_backtest_engine_config returns correct "
          "BacktestEngineConfig",
          "[config][backtest_engine_config]") {
    using namespace utils::config;
    using namespace core::backtest;

    const std::string config_file = "test_backtest_engine_config.tmp";
    {
        std::ofstream out(config_file);
        out << "initial_cash=5000.0\n"
            << "order_entry_latency_us=12345\n"
            << "order_response_latency_us=23456\n"
            << "market_feed_latency_us=34567\n";
    }

    ConfigReader reader;
    const auto config =
        reader.get_backtest_engine_config(config_file);

    REQUIRE(config.initial_cash_ == 5000.0);
    REQUIRE(config.order_entry_latency_us_ == 12345);
    REQUIRE(config.order_response_latency_us_ == 23456);
    REQUIRE(config.market_feed_latency_us_ == 34567);

    std::filesystem::remove(config_file);
}

TEST_CASE("[ConfigReader] - get_backtest_engine_config throws on missing keys",
          "[config][backtest_engine_config][missing]") {
    using namespace utils::config;
    using namespace core::backtest;

    const std::string config_file = "test_backtest_engine_config_missing.tmp";
    // Omit some required keys
    {
        std::ofstream out(config_file);
        out << "initial_cash=5000.0\n"
            // Missing order_entry_latency_us
            << "order_response_latency_us=23456\n"
            << "market_feed_latency_us=34567\n";
    }

    ConfigReader reader;
    REQUIRE_THROWS_AS(reader.get_backtest_engine_config(config_file),
                      std::runtime_error);

    std::filesystem::remove(config_file);
}

TEST_CASE("[ConfigReader] - get_recorder_config returns correct RecorderConfig",
          "[config][recorder_config]") {
    using namespace utils::config;

    const std::string config_file = "test_recorder_config.tmp";
    {
        std::ofstream out(config_file);
        out << "interval_us=500000\n"
            << "output_file=test_output.csv\n";
    }

    ConfigReader reader;
    RecorderConfig config = reader.get_recorder_config(config_file);

    REQUIRE(config.interval_us == 500000);
    REQUIRE(config.output_file == "test_output.csv");

    std::filesystem::remove(config_file);
}

TEST_CASE("[ConfigReader] - get_recorder_config uses defaults if missing",
          "[config][recorder_config][defaults]") {
    using namespace utils::config;

    const std::string config_file = "test_recorder_config_defaults.tmp";
    {
        std::ofstream out(config_file);
        // Only provide interval_us
        out << "interval_us=250000\n";
    }

    ConfigReader reader;
    RecorderConfig config = reader.get_recorder_config(config_file);

    REQUIRE(config.interval_us == 250000);
    REQUIRE(config.output_file == "recorder_output.csv");

    std::filesystem::remove(config_file);
}

TEST_CASE("[ConfigReader] - get_backtest_config returns correct BacktestConfig",
          "[config][backtest_config]") {
    using namespace utils::config;

    const std::string config_file = "test_backtest_config.tmp";
    {
        std::ofstream out(config_file);
        out << "elapse_us=2000000\n"
            << "iterations=12345\n";
    }

    ConfigReader reader;
    core::backtest::BacktestConfig config =
        reader.get_backtest_config(config_file);

    REQUIRE(config.elapse_us == 2000000);
    REQUIRE(config.iterations == 12345);

    std::filesystem::remove(config_file);
}

TEST_CASE("[ConfigReader] - get_backtest_config uses defaults if missing",
          "[config][backtest_config][defaults]") {
    using namespace utils::config;

    const std::string config_file = "test_backtest_config_defaults.tmp";
    {
        std::ofstream out(config_file);
        // Only provide elapse_us
        out << "elapse_us=500000\n";
    }

    ConfigReader reader;
    core::backtest::BacktestConfig config =
        reader.get_backtest_config(config_file);

    REQUIRE(config.elapse_us == 500000);
    REQUIRE(config.iterations == 86400); // default value

    std::filesystem::remove(config_file);
}