/*
 * File: hftengine/core/strategy/strategy.cpp
 * Description: Base class for strategies.
 * Author: Arvind Rathnashyam
 * Date: 2025-08-09
 * License: Proprietary
 */

#pragma once

#include <memory>
#include <vector>

#include "../../utils/logger/logger.h"
#include "../backtest_engine/backtest_engine.h"
#include "../types/aliases/usings.h"

namespace core::strategy {
class Strategy {
  public:
    explicit Strategy() {};

    virtual ~Strategy() = default;

    virtual void initialize() = 0;
    virtual void on_elapse(core::backtest::BacktestEngine &hbt) = 0;

  private:
};
} // namespace core::strategy