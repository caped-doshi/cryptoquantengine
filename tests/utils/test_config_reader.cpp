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

#include "core/strategy/grid_trading_config.h"
#include "core/trading/asset_config.h"
#include "core/trading/backtest_engine_config.h"
#include "utils/config/config_reader.h"

TEST_CASE("[ConfigReader] - get_asset_config returns correct AssetConfig",
          "[config][asset_config]") {
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
    AssetConfig config = reader.get_asset_config(config_file);

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
    GridTradingConfig config = reader.get_grid_trading_config(config_file);

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
    const std::string config_file = "test_backtest_engine_config.tmp";
    {
        std::ofstream out(config_file);
        out << "initial_cash=5000.0\n"
            << "order_entry_latency_us=12345\n"
            << "order_response_latency_us=23456\n"
            << "market_feed_latency_us=34567\n";
    }

    ConfigReader reader;
    BacktestEngineConfig config =
        reader.get_backtest_engine_config(config_file);

    REQUIRE(config.initial_cash_ == 5000.0);
    REQUIRE(config.order_entry_latency_us_ == 12345);
    REQUIRE(config.order_response_latency_us_ == 23456);
    REQUIRE(config.market_feed_latency_us_ == 34567);

    std::filesystem::remove(config_file);
}

TEST_CASE("[ConfigReader] - get_backtest_engine_config throws on missing keys",
          "[config][backtest_engine_config][missing]") {
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