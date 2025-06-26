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
	Timestamp timestamp_;
	Timestamp local_timestamp_;
	TradeSide side_;
	Price price_;
	Quantity quantity_;
	OrderId orderId_;
};