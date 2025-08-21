/*
 * Copyright (c) 2025 arvindkrv@protonmail.com
 *
 * Please see the LICENSE file for the terms and conditions
 * associated with this software.
 */

#pragma once

#include <cstdint>

namespace core::backtest {
struct BacktestEngineConfig {
    double initial_cash_ = 1000.0;
    std::uint64_t order_entry_latency_us_ = 25000;
    std::uint64_t order_response_latency_us_ = 25000;
    std::uint64_t market_feed_latency_us_ = 50000;
};
} 