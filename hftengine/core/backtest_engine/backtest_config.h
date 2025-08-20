/*
 * File: hftengine/core/backtest_engine/backtest_config.h
 * Description: Configuration for backtest simulation, including elapse time
 * and number of iterations.
 * Author: Arvind Rathnashyam
 * Date: 2025-08-19
 * License: Proprietary
 */

#pragma once

#include <cstdint>

struct BacktestConfig {
    std::uint64_t elapse_us = 1'000'000;
    std::uint64_t iterations = 86'400;
};