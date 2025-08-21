/*
 * File: hftengine/hftengine/core/strategy/grid_trading/gridtrading_config.h
 * Description:  
 * Author: Arvind Rathnashyam
 * Date: 2025-06-24 
 * License: Proprietary
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