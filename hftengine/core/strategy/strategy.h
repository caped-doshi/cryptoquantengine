/*
 * File: hftengine/core/strategy/strategy.cpp
 * Description: Base class for strategies. 
 * Author: Arvind Rathnashyam
 * Date: 2025-08-09
 * License: Proprietary
 */

#pragma once 

#include <vector>
#include <memory>

#include "../types/usings.h"
#include "../trading/backtest_engine.h"

class Strategy {
  public:
    explicit Strategy() {};

    virtual ~Strategy() = default;

    virtual void initialize() = 0;
    virtual void on_elapse(BacktestEngine& hbt) = 0;
};