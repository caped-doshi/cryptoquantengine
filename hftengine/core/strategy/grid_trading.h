/*
 * File: hftengine/core/strategy/grid_trading.h
 * Description: Basic grid market making strategy with no alpha.
 * Author: Arvind Rathnashyam
 * Date: 2025-08-10
 * License: Proprietary
 */

#pragma once

#include <cmath>
#include <memory>
#include <vector>

#include "../../utils/logger/logger.h"
#include "../trading/backtest_engine.h"
#include "../trading/depth.h"
#include "../types/usings.h"
#include "grid_trading_config.h"
#include "strategy.h"

class GridTrading : public Strategy {
  public:
    explicit GridTrading(int asset_id, int grid_num, Ticks grid_interval,
                         Ticks half_spread, double position_limit,
                         double notional_order_qty,
                         std::shared_ptr<Logger> logger);
    explicit GridTrading(int asset_id, const GridTradingConfig &config,
                         std::shared_ptr<Logger> logger = nullptr);

    void initialize() override;
    void on_elapse(BacktestEngine &hbt) override;

  private:
    int asset_id_;
    int grid_num_;
    Ticks grid_interval_;
    Ticks half_spread_;
    double position_limit_;
    double notional_order_qty_;

    std::shared_ptr<Logger> logger_;
};