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

# include "utils/config/config_reader.hpp"

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