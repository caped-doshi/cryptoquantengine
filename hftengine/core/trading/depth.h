/*
 * File: hftengine/core/trading/depth.h
 * Description: Level two price data, tick size, and lot size for an asset.
 * Author: Arvind Rathnashyam
 * Date: 2025-07-01
 * License: Proprietary
 */

#pragma once

#include <unordered_map>

#include "../types/usings.h"

/**
 * @brief Represents the best bid and ask prices and their quantities.
 */
struct Depth {
    Ticks best_bid_ = 0;
    Quantity bid_qty_ = 0.0;
    Ticks best_ask_ = 0;
    Quantity ask_qty_ = 0.0;
    std::map<Ticks, Quantity, std::greater<>> bid_depth_;
    std::map<Ticks, Quantity> ask_depth_;
    double tick_size_;
    double lot_size_;
};