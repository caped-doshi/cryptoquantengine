/*
 * File: hftengine/core/recorder/recorder.cpp
 * Description: Class to record equities and return performance metrics
 * such as annualized Sharpe, Sortino, and max drawdown.
 * Author: Arvind Rathnashyam
 * Date: 2025-07-07
 * License: Proprietary
 */

#include <algorithm>
#include <cstdlib>
#include <fstream>
#include <iostream>
#include <iterator>
#include <memory>
#include <stdexcept>
#include <vector>

#include "../../utils/logger/logger.h"
#include "../../utils/logger/log_level.h"
#include "../../utils/math/math_utils.h"
#include "../../utils/stat/stat_utils.h"
#include "../types/usings.h"
#include "recorder.h"

/**
 * @brief Constructs a Recorder with a specified interval.
 *
 * This constructor initializes the Recorder with a given time interval in
 * microseconds, which will be used to calculate returns and risk metrics.
 *
 * @param interval_us The time interval in microseconds for recording equity
 * snapshots.
 * @param logger Optional shared pointer to a Logger instance for logging
 */
Recorder::Recorder(Microseconds interval_us, std::shared_ptr<Logger> logger)
    : interval_us_(interval_us), logger_(logger) {
    if (logger_) {
        logger_->log("[Recorder] - Initialized with interval: " +
                         std::to_string(interval_us) + " microseconds",
                     LogLevel::Debug);
    }
}

/**
 * @brief Records an equity snapshot.
 *
 * This method appends a new equity snapshot to the internal records vector.
 *
 * @param snapshot The equity snapshot to record.
 */
void Recorder::record(const EquitySnapshot &snapshot) {
    records_.emplace_back(snapshot);
}

/**
 * @brief Records an equity snapshot at a given timestamp.
 *
 * This method appends a new equity snapshot to the internal records vector.
 *
 * @param timestamp The timestamp of the equity snapshot.
 * @param equity The equity value at the given timestamp.
 */
void Recorder::record(Timestamp timestamp, double equity) {
    records_.emplace_back(EquitySnapshot{timestamp, equity});
}

/**
 * @brief Records the current equity and position for a specific asset.
 *
 * This method captures the current equity and position of the specified asset
 * in the backtest engine and appends it to the internal state records vector.
 *
 * @param hbt The BacktestEngine instance containing the current state.
 * @param asset_id The ID of the asset to record.
 */
void Recorder::record(const BacktestEngine &hbt, int asset_id) {
    Timestamp current_time = hbt.current_time();
    double equity = hbt.equity();
    Quantity position = hbt.position(asset_id);
    Depth depth = hbt.depth(asset_id);
    double tick_size = depth.tick_size_;
    Price mid_price = (ticks_to_price(depth.best_bid_, tick_size) +
                       ticks_to_price(depth.best_ask_, tick_size)) /
                      2.0;
    if (!std::isfinite(mid_price)) mid_price = 0.0;

    records_.emplace_back(EquitySnapshot{current_time, equity});
    state_records_.emplace_back(
        StateSnapshot{current_time, equity, position, mid_price});
    
    if (logger_) {
        logger_->log("[Recorder] - " + std::to_string(current_time) +
                         "us - asset_id= " + std::to_string(asset_id) +
                         ", equity=" + std::to_string(equity) +
                         ", position=" + std::to_string(position) +
                         ", price=" + std::to_string(mid_price),
                     LogLevel::Debug);
    }
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

    for (Timestamp t = start_time; t <= end_time; t += interval_us_) {
        while (i + 1 < records_.size() &&
               records_[i + 1].timestamp_ <= t + interval_us_) {
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
 * @brief Returns the annualized Sharpe ratio from chronologically increasing
 * equities.
 *
 * The Sharpe ratio is a measure of risk-adjusted return, calculated as the
 * mean return divided by the standard deviation of returns, scaled by an
 * annualization factor.
 *
 * @return Sharpe ratio value.
 * @throws std::runtime_error if no returns data is available or if standard
 * deviation is zero.
 */
double Recorder::sharpe() const {
    std::vector<double> returns = interval_returns();

    if (returns.empty()) {
        throw std::runtime_error(
            "Cannot calculate Sharpe ratio: no returns data");
    }

    long double ann_factor =
        sqrt((365 * 24 * 60 * 60) / (interval_us_ / 1'000'000.0));
    double ret_mean = mean(returns);
    double ret_stddev = stddev(returns);

    if (std::abs(ret_stddev) <= 1e-9) {
        throw std::runtime_error("Cannot calculate Sharpe ratio: standard "
                                 "deviation too close to zero");
    }
    return ann_factor * mean(returns) / stddev(returns);
}

/**
 * @brief Returns the Sortino value from chronologically increasing equities.
 *
 * Sortino ratio is a modification of the Sharpe ratio that only considers
 * downside risk (negative returns).
 *
 * @return Sortino value.
 * @throws std::runtime_error if no negative returns are available
 * or if downside deviation is zero.
 */
double Recorder::sortino() const {
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
double Recorder::max_drawdown() const {
    if (records_.empty()) {
        throw std::runtime_error(
            "Cannot calculate max drawdown : no records available");
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

void Recorder::plot(int asset_id) const {
    std::string csv_filename =
        "recorder_plot_" + std::to_string(asset_id) + ".csv";
    std::ofstream csv(csv_filename);
    csv << "timestamp,equity,position,mid_price\n";
    for (const auto &state : state_records_) {
        if (state.price_ <= 0) continue;
        csv << state.timestamp_ << "," << state.equity_ << ","
            << state.position_ << "," << state.price_ << "\n";
    }
    csv.close();

    std::string command =
        "python ../hftengine/core/recorder/plot_recorder.py " + csv_filename +
        " " + std::to_string(asset_id);
    std::system(command.c_str());
}