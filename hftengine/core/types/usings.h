/*
 * File: hft_bt_engine/core/market_data/usings.h
 * Description: Type aliases for price, quantity, timestamp, and IDs used in the order book and trade system.
 * Author: Arvind Rathnashyam
 * Date: 2025-06-23
 * License: Proprietary
 */

#pragma once

# include <cstdint>
# include <variant>
# include <vector>

 // Forward declarations
struct BookUpdate;
struct Trade;

using QuantityLot = std::uint32_t;
using Timestamp = std::uint64_t;
using OrderId = std::uint64_t;
using Ticks = std::uint64_t;
using Price = double;
using Quantity = double;
using Position = double;
using Microseconds = std::uint64_t;
using MarketEvent = std::variant<BookUpdate, Trade>;