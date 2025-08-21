/*
 * Copyright (c) 2025 arvindkrv@protonmail.com
 *
 * Please see the LICENSE file for the terms and conditions
 * associated with this software.
 */

#pragma once

#include <cstdint>
#include <variant>

using Timestamp = std::uint64_t;
using OrderId = std::uint64_t;
using Ticks = std::uint64_t;
using Price = double;
using Quantity = double;
using Position = double;
using Microseconds = std::uint64_t;

namespace core::market_data {
struct BookUpdate;
struct Trade;
} // namespace core::market_data

using MarketEvent =
    std::variant<core::market_data::BookUpdate, core::market_data::Trade>;