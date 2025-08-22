/*
 * Copyright (c) 2025 arvindkrv@protonmail.com
 *
 * Please see the LICENSE file for the terms and conditions
 * associated with this software.
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
    virtual void on_elapse(core::backtest::BacktestEngine &engine) = 0;

  private:
};
} // namespace core::strategy