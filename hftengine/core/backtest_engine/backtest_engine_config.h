/*
 * File: hftengine/core/backtest_engine/backtest_engine_config.h
 * Description: Configuration for the backtest engine, including initial cash,
 * order entry latency, order response latency, and market feed latency.
 * Author: Arvind Rathnashyam
 * Date: 2025-08-18
 * License: Proprietary
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