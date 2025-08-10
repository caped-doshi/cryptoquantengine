/*
 * File: hftengine/core/recorder/recorder.cpp
 * Description: Class to record equities and return performance metrics
 * such as annualized Sharpe, Sortino, and max drawdown.
 * Author: Arvind Rathnashyam
 * Date: 2025-07-07
 * License: Proprietary
 */

#include <vector>
#include <algorithm>
#include <iterator>
#include <stdexcept>

#include "../../utils/stat/stat_utils.h"
#include "../types/usings.h"
#include "recorder.h"

Recorder::Recorder(Microseconds interval_us) : interval_us_(interval_us) {}

void Recorder::record(const EquitySnapshot &snapshot) { records_.emplace_back(snapshot); }

void Recorder::record(Timestamp timestamp, double equity) {
    records_.emplace_back(EquitySnapshot{timestamp, equity});
}

/**
 * @brief gets returns between intervals
 * 
 * Equity snapshots are from non-uniform time intervals. To calculate
 * the risk ratios (e.g. sharpe), we take the equity at pre-determined
 * intervals and then put the returns in a vector. 
 * 
 * @return vector of double returns. 
 */
std::vector<double> Recorder::interval_returns() const {
    std::vector<double> returns;
    if (records_.size() < 2) return returns;

    Timestamp start_time = records_.front().timestamp_;
    Timestamp end_time = records_.back().timestamp_;
    double last_value = records_.front().equity_;
    double current_value = records_.front().equity_;
    std::size_t i = 0;

    for (Timestamp t = start_time; t <= end_time;
         t += interval_us_) {
        while (i + 1 < records_.size() && records_[i + 1].timestamp_ <= t + interval_us_) {
            ++i;
        }
        current_value = records_[i].equity_;
        if (last_value > 0.0)
            returns.emplace_back((current_value - last_value) / last_value);
        else
            returns.push_back(0.0);

        last_value = current_value;
    }
    return returns;
}

/**
 * @brief Returns the sharpe value from chronologically increasing equities. 
 * 
 * @return Sharpe value. 
 */
const double Recorder::sharpe() const {
    std::vector<double> returns = interval_returns();

    if (returns.empty()) {
        throw std::runtime_error(
            "Cannot calculate Sharpe ratio: no returns data");
    }

    long double ann_factor = sqrt((365 * 24 * 60 * 60) / (interval_us_ / 1'000'000.0));
    double ret_mean = mean(returns);
    double ret_stddev = stddev(returns);

    if (std::abs(ret_stddev) <= 1e-9) {
        throw std::runtime_error(
            "Cannot calculate Sharpe ratio: no returns data");    
    }
    return ann_factor * mean(returns) / stddev(returns);
}

/**
 * @brief Returns the sortino ratio. 
 * 
 * @return Sortino value.
 */ 
const double Recorder::sortino() const {
    std::vector<double> returns = interval_returns();
    std::vector<double> returns_neg;
    std::copy_if(returns.begin(), returns.end(),
                 std::back_inserter(returns_neg),
                 [](double num) { return num < 0; });

    if (returns_neg.empty()) {
        throw std::runtime_error(
            "Cannot calculate Sortino ratio : no positive returns");
    }

    long double ann_factor =
        sqrt((365 * 24 * 60 * 60) / (interval_us_ / 1'000'000.0));
    double ret_mean = mean(returns);
    double ret_stddev_neg = stddev(returns_neg);

    if (std::abs(ret_stddev_neg) <= 1e-9) {
        throw std::runtime_error(
            "Cannot calculate Sortino ratio : downside deviation is zero");   
    }

    return ann_factor * mean(returns) / stddev(returns_neg);
}

/**
 * @brief Calculates the max drawdown from recorded equity values
 * 
 * Max drawdown is the largest peak-to-trough decline in equty value, 
 * expressed as a percentage of the peak value. 
 * 
 * @return maximum drawdown of as a percentage (0.0 to 1.0)
 * @throws std::runtime_error if no records are available
 */
const double Recorder::max_drawdown() const {
    if (records_.empty()) {
        throw std::runtime_error("Cannot calculate max drawdown : no records available");
    }

    double peak = records_.front().equity_;
    double max_dd = 0.0;

    for (const auto &snapshot : records_) {
        if (snapshot.equity_ > peak) {
            peak = snapshot.equity_;
        } else {
            double drawdown = (peak - snapshot.equity_) / peak;
            if (drawdown > max_dd) {
                max_dd = drawdown;
            }
        }
    }
    return max_dd;
}