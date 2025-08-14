/*
 * File: hftengine/core/strategy/grid_trading.h
 * Description: Basic grid market making strategy with no alpha.
 * Author: Arvind Rathnashyam
 * Date: 2025-08-10
 * License: Proprietary
 */

#pragma once

#include <cmath>
#include <vector>

#include "../trading/backtest_engine.h"
#include "../trading/depth.h"
#include "../types/usings.h"
#include "strategy.h"

class GridTrading : public Strategy {
  public:
    explicit GridTrading(const int asset_id, const int grid_num,
                         const Ticks grid_interval, const Ticks half_spread,
                         const double position_limit,
                         const double notional_order_qty);

    void initialize() override;
    void on_elapse(BacktestEngine &hbt) override;

  private:
    int asset_id_;
    int grid_num_;
    Ticks grid_interval_;
    Ticks half_spread_;
    double position_limit_;
    double notional_order_qty_;
};