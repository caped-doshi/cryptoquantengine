/*
 * File: tests/test_stat_utils.cpp
 * Description: Unit tests for the stat utils functions. 
 * Author: Arvind Rathnashyam
 * Date: 2025-07-25
 * License: Proprietary
 */

#include <catch2/catch_approx.hpp>
#include <catch2/catch_test_macros.hpp>
#include <filesystem>
#include <fstream>
#include <memory>
#include <vector>
#include <cmath>
#include <numeric>

#include "../hftengine/utils/stat/stat_utils.h"

TEST_CASE("[StatUtils] - mean() computes arithmetic mean", "[statutils][mean]") {
    SECTION("basic cases") {
        REQUIRE(mean({1.0, 2.0, 3.0, 4.0, 5.0}) ==
                Catch::Approx(3.0).margin(1e-8));
        REQUIRE(mean({-1.0, 0.0, 1.0}) == Catch::Approx(0.0).margin(1e-8));
        REQUIRE(mean({10.0, 10.0, 10.0}) == Catch::Approx(10.0).margin(1e-8));
    }

    SECTION("single element") {
        REQUIRE(mean({42.0}) == Catch::Approx(42.0).margin(1e-8));
    }

    SECTION("empty vector throws (or handle as you prefer)") {
        // Currently would throw (division by zero) - choose either to test for
        // throw or handle like stddev
        REQUIRE_THROWS(mean(std::vector<double>{}));

        // If you modify to handle empty case like stddev does:
        // REQUIRE(mean(std::vector<double>{}) == Catch::Approx(0.0));
    }

    SECTION("floating point precision") {
        REQUIRE(mean({1.0, 1.0, 1.0, 1.0, 1.00000001}) ==
                Catch::Approx(1.000000002).margin(1e-8));
    }
}

TEST_CASE("[StatUtils] - stddev() computes population standard deviation",
          "[statutils][stddev]") {
    SECTION("basic cases") {
        REQUIRE(stddev({1.0, 1.0, 1.0, 1.0}) ==
                Catch::Approx(0.0).margin(1e-8));
        REQUIRE(stddev({2.0, 4.0, 4.0, 4.0, 5.0, 5.0, 7.0, 9.0}) ==
                Catch::Approx(2.0).margin(1e-8));
        REQUIRE(stddev({-2.0, -1.0, 0.0, 1.0, 2.0}) ==
                Catch::Approx(1.414213562).margin(1e-8));
    }

    SECTION("edge cases") {
        REQUIRE(stddev(std::vector<double>{}) == Catch::Approx(0.0));
        REQUIRE(stddev({5.0}) == Catch::Approx(0.0));
    }

    SECTION("floating point precision") {
        auto result =
            stddev({10000000000.0, 10000000000.00001, 10000000000.00001});
        REQUIRE(result == Catch::Approx(0.0000045403979165032).margin(1e-8));
    }
}