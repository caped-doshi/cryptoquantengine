/*
 * File: hft_bt_engine/core/market_data/levelData.h
 * Description: Defines the Trade struct representing an executed trade in the market.
 *              Includes timestamp, side (buy/sell), price, quantity, and associated order ID.
 * Author: Arvind Rathnashyam
 * Date: 2025-06-23
 * License: Proprietary
 */

#pragma once

#include "side.h"
#include "usings.h"

struct LevelData {
	Quantity quantity_;
	Price price_;
};