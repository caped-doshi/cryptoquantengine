/*
 * File: hftengine/core/recorder/recorder.h
 * Description: Class to record equities and return performance metrics
 * such as annualized Sharpe, Sortino, and max drawdown.
 * Author: Arvind Rathnashyam
 * Date: 2025-07-07
 * License: Proprietary
 */

#pragma once

#include <memory>
#include <vector>

#include "../../utils/logger/logger.h"
#include "../trading/backtest_engine.h"
#include "../types/usings.h"
#include "equity_snapshot.h"
#include "state_snapshot.h"

class Recorder {
  public:
    Recorder(Microseconds interval_us,
             std::shared_ptr<Logger> logger = nullptr);

    void record(const EquitySnapshot &snapshot);
    void record(Timestamp timestamp, double equity);
    void record(const BacktestEngine &hbt, int asset_id);

    double sharpe() const;
    double sortino() const;
    double max_drawdown() const;

    void print_performance_metrics() const;
    void plot(int asset_id) const;

    std::vector<double> interval_returns() const;

  private:
    Microseconds interval_us_;
    std::vector<EquitySnapshot> records_;
    std::vector<StateSnapshot> state_records_;

    std::shared_ptr<Logger> logger_;
};