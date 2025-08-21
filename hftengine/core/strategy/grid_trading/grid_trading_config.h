/*
 * Copyright (c) 2025 arvindkrv@protonmail.com
 *
 * Please see the LICENSE file for the terms and conditions
 * associated with this software.
 */

#pragma once

#include "../../types/aliases/usings.h" 

namespace core::strategy {
struct GridTradingConfig {
    double tick_size_;
    double lot_size_;
    int grid_num_;
    Ticks grid_interval_;
    Ticks half_spread_;
    double position_limit_;
    double notional_order_qty_;
};
} // namespace core::strategy