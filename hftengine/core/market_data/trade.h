/*
 * File: hft_bt_engine/core/market_data/trade.h
 * Description: Defines the Trade struct representing an executed trade in the market.
 *              Includes timestamp, side (buy/sell), price, quantity, and associated order ID.
 * Author: Arvind Rathnashyam
 * Date: 2025-06-23
 * License: Proprietary
 */

#pragma once

# include "../types/usings.h"
# include "../types/trade_side.h"

struct Trade {
	Timestamp exch_timestamp_;    // arrives at exchange first
	Timestamp local_timestamp_;   // then sent to local with latency
	TradeSide side_;
	Price price_;
	Quantity quantity_;
	OrderId orderId_;
};