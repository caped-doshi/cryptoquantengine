/*
 * File: hftengine/core/recorder/recorder.h
 * Description: Class to record equities and return performance metrics 
 * such as annualized Sharpe, Sortino, and max drawdown. 
 * Author: Arvind Rathnashyam
 * Date: 2025-07-07
 * License: Proprietary
 */

#pragma once

#include <vector>

#include "../types/usings.h"
#include "equity_snapshot.h"

class Recorder {
  public:
    Recorder(Microseconds interval_us);

    void record(const EquitySnapshot &snapshot);
    void record(Timestamp timestamp, double equity);

    const double sharpe() const;
    const double sortino() const;
    const double max_drawdown() const;

    std::vector<double> interval_returns() const;

  private:
    Microseconds interval_us_;
    std::vector<EquitySnapshot> records_;
};