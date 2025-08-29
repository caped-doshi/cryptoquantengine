/*
 * Copyright (c) 2025 arvindkrv@protonmail.com
 *
 * Please see the LICENSE file for the terms and conditions
 * associated with this software.
 */

#pragma once

#include <unordered_map>

#include "../types/aliases/usings.h"

namespace core::trading {
struct Depth {
    Ticks best_bid_ = 0;
    Quantity bid_qty_ = 0.0;
    Ticks best_ask_ = 0;
    Quantity ask_qty_ = 0.0;
    std::unordered_map<Ticks, Quantity> bid_depth_;
    std::unordered_map<Ticks, Quantity> ask_depth_;
    double tick_size_;
    double lot_size_;
};
}