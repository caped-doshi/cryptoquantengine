/*
 * File: tests/utils/test_math_utils.cpp
 * Description: Unit tests for math utility functions (price/tick conversions,
 * lot rounding).
 * Author: Arvind Rathnashyam
 * Date: 2025-08-13
 * License: Proprietary
 */

#include "utils/math/math_utils.h"
#include <catch2/catch_approx.hpp>
#include <catch2/catch_test_macros.hpp>

TEST_CASE("[MathUtils] - price_to_ticks converts price to ticks correctly",
          "[math_utils][price_to_ticks]") {
    using namespace utils::math;
    double tick_size = 0.01;
    REQUIRE(price_to_ticks(100.00, tick_size) == 10000);
    REQUIRE(price_to_ticks(100.005, tick_size) ==
            10001); // 100.005/0.01 = 10000.5, rounds to 10001
    REQUIRE(price_to_ticks(0.0, tick_size) == 0);
    REQUIRE(price_to_ticks(0.009, tick_size) == 1); // rounds up
}

TEST_CASE("[MathUtils] - ticks_to_price converts ticks to price correctly",
          "[math_utils][ticks_to_price]") {
    using namespace utils::math;
    double tick_size = 0.01;
    REQUIRE(ticks_to_price(10000, tick_size) ==
            Catch::Approx(100.00).margin(1e-8));
    REQUIRE(ticks_to_price(10001, tick_size) ==
            Catch::Approx(100.01).margin(1e-8));
    REQUIRE(ticks_to_price(0, tick_size) == Catch::Approx(0.0).margin(1e-8));
}

TEST_CASE("[MathUtils] - quantity_to_lot rounds quantity to nearest lot size",
          "[math_utils][quantity_to_lots]") {
    using namespace utils::math;
    double lot_size = 0.01;
    REQUIRE(quantity_to_lot(1.005, lot_size) ==
            Catch::Approx(1.01).margin(1e-8));
    REQUIRE(quantity_to_lot(1.004, lot_size) ==
            Catch::Approx(1.00).margin(1e-8));
    REQUIRE(quantity_to_lot(0.0, lot_size) == Catch::Approx(0.0).margin(1e-8));
    REQUIRE(quantity_to_lot(0.009, lot_size) ==
            Catch::Approx(0.01).margin(1e-8));
}