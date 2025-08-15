/*
 * File: tests/test_config_reader.cpp
 * Description: Unit tests for hft/utils/config/config_reader.cpp.
 * Author: Arvind Rathnashyam
 * Date: 2025-06-25
 * License: Proprietary
 */

# include <catch2/catch_test_macros.hpp>
# include <filesystem>
# include <fstream>

# include "utils/config/config_reader.h"

/*TEST_CASE("[ConfigReader] - Edge Cases",
               "[config][edge]"){
    SECTION("Handles empty file") {
        const std::string empty_file = "empty_config.tmp";
        {
            std::ofstream out(empty_file);
        }

        ConfigReader reader(empty_file);
        REQUIRE_FALSE(reader.has("any_key"));
        std::filesystem::remove(empty_file);
    }

    SECTION("Handles duplicate keys (last wins)") {
        const std::string dup_file = "dup_config.tmp";
        {
            std::ofstream out(dup_file);
            out << "key=first\n"
                << "key=last\n";
        }

        ConfigReader reader(dup_file);
        REQUIRE(reader.get_string("key") == "last");
        std::filesystem::remove(dup_file);
    }
}*/

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
            << "position_limit=10.0\n";
    }

    ConfigReader reader;
    GridTradingConfig config = reader.get_grid_trading_config(config_file);

    REQUIRE(config.tick_size == 0.01);
    REQUIRE(config.lot_size == 0.00001);
    REQUIRE(config.grid_num == 3);
    REQUIRE(config.grid_interval == 10);
    REQUIRE(config.half_spread == 20);
    REQUIRE(config.position_limit == 10.0);

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
            << "position_limit=10.0\n";
    }

    ConfigReader reader;
    REQUIRE_THROWS_AS(reader.get_grid_trading_config(config_file), std::runtime_error);

    std::filesystem::remove(config_file);
}