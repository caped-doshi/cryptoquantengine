/*
 * File: hftengine/core/trading/depth.h
 * Description: Level one data for an asset.
 * Author: Arvind Rathnashyam
 * Date: 2025-07-01
 * License: Proprietary
 */

#pragma once

#include "../types/usings.h" 

/**
 * @brief Represents the best bid and ask prices and their quantities.
 */
struct Depth {
    Price best_bid = 0.0;
    Quantity bid_qty = 0.0;
    Price best_ask = 0.0;
    Quantity ask_qty = 0.0;
};