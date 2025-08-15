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

TEST_CASE("[ConfigReader] - File Handling", "[config][file]") {
    const std::string test_file = "test_config.tmp";

    SECTION("Successfully opens valid file") {
        {
            std::ofstream out(test_file);
            out << "test_key=test_value\n";
        }

        ConfigReader reader(test_file);
        REQUIRE_NOTHROW(reader);
        std::filesystem::remove(test_file);
    }

    SECTION("Throws on nonexistent file") {
        REQUIRE_THROWS_AS(ConfigReader("nonexistent_file.txt"), std::runtime_error);
    }
}

TEST_CASE("[ConfigReader] - Value Parsing", "[config][parsing]") {
    const std::string test_file = "test_config.tmp";
    {
        std::ofstream out(test_file);
        out << "string_val=hello\n"
            << "int_val=42\n"
            << "double_val=3.1415\n"
            << "empty_val=\n"
            << "# Comment line\n"
            << "malformed_line\n";
    }

    ConfigReader reader(test_file);

    SECTION("Correctly reads string values") {
        REQUIRE(reader.get_string("string_val") == "hello");
    }

    SECTION("Correctly reads numeric values") {
        REQUIRE(reader.get_int("int_val") == 42);
        REQUIRE(reader.get_double("double_val") == 3.1415);
    }

    SECTION("Has key detection works") {
        REQUIRE(reader.has("string_val"));
        REQUIRE_FALSE(reader.has("nonexistent_key"));
        REQUIRE_FALSE(reader.has("malformed_line"));
        REQUIRE_FALSE(reader.has("empty_val"));
    }

    SECTION("Throws on missing keys") {
        REQUIRE_THROWS_AS(reader.get_string("nonexistent"), std::runtime_error);
    }

    SECTION("Throws on type conversion errors") {
        REQUIRE_THROWS_AS(reader.get_int("string_val"), std::invalid_argument);
        REQUIRE_THROWS_AS(reader.get_double("string_val"), std::invalid_argument);
    }

    std::filesystem::remove(test_file);
}

TEST_CASE("[ConfigReader] - Edge Cases", "[config][edge]") {
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
}

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

    ConfigReader reader(config_file);
    AssetConfig config = reader.get_asset_config();

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