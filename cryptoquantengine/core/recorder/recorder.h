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
#include "equity_snapshot.h"
#include "state_snapshot.h"

namespace core::recorder {
class Recorder {
  public:
    Recorder(Microseconds interval_us,
             std::shared_ptr<utils::logger::Logger> logger = nullptr);

    void record(const EquitySnapshot &snapshot);
    void record(Timestamp timestamp, double equity);
    void record(const core::backtest::BacktestEngine &engine, int asset_id);

    double sharpe() const;
    double sortino() const;
    double max_drawdown() const;

    void print_performance_metrics() const;
    void plot(const std::string& asset_name) const;

    std::vector<double> interval_returns() const;

  private:
    Microseconds interval_us_;
    std::vector<EquitySnapshot> records_;
    std::vector<StateSnapshot> state_records_;

    std::shared_ptr<utils::logger::Logger> logger_;
};
} // namespace core::recorder