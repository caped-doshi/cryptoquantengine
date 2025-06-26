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

 // Forward declarations
struct L2Update;
struct Trade;

using PriceTick = std::uint32_t;
using QuantityLot = std::uint32_t;
using Timestamp = std::uint64_t;
using OrderId = std::uint64_t;
using Price = double;
using Quantity = double;

using MarketEvent = std::variant<L2Update, Trade>;