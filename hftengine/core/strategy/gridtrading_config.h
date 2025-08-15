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
    double tick_size;
    double lot_size;
    int grid_num;
    Ticks grid_interval;
    Ticks half_spread;
    double position_limit;
};