/*
 * Copyright (c) 2025 arvindkrv@protonmail.com
 *
 * Please see the LICENSE file for the terms and conditions
 * associated with this software.
 */

#pragma once

#include <cstdint>

namespace core::backtest {
struct BacktestConfig {
    std::uint64_t elapse_us = 1'000'000;
    std::uint64_t iterations = 86'400;
};
} // namespace core::backtest