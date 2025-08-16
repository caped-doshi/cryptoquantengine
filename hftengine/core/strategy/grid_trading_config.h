/*
 * File: hftengine/hftengine/core/strategy/gridtrading_config.h
 * Description:  
 * Author: Arvind Rathnashyam
 * Date: 2025-06-24 
 * License: Proprietary
 */

#pragma once

#include "../types/usings.h" 

struct GridTradingConfig {
    double tick_size_;
    double lot_size_;
    int grid_num_;
    Ticks grid_interval_;
    Ticks half_spread_;
    double position_limit_;
    double notional_order_qty_;
};