/*
 * Copyright (c) 2025 arvindkrv@protonmail.com
 *
 * Please see the LICENSE file for the terms and conditions
 * associated with this software.
 */

#pragma once

#include <vector>
#include <cmath>
#include <numeric>
#include <stdexcept>

#include "../../core/types/aliases/usings.h"

namespace utils::stat {
/**
 * @brief Computes the mean of a vector of doubles.
 *
 * @param data A vector of doubles.
 * @return mean of data.
 */
inline double mean(const std::vector<double> &data) {
    if (data.empty())
        throw std::invalid_argument("Cannot compute mean of empty vector");
    if (data.size() > std::numeric_limits<double>::max())
        throw std::overflow_error("Input size too large for mean calculation");
    double mean = std::accumulate(data.begin(), data.end(), 0.0) / data.size();
    return mean;
}

/**
 * @brief Computes the population standard deviation of a series.
 *
 * @param data A vector of values.
 * @return Standard deviation of the input values.
 */
inline double stddev(const std::vector<double> &data) {
    if (data.empty()) return 0.0;

    double mean = std::accumulate(data.begin(), data.end(), 0.0) / data.size();
    double sq_sum = 0.0;
    for (double x : data) {
        sq_sum += (x - mean) * (x - mean);
    }
    return std::sqrt(sq_sum / (data.size()));
}
} // namespace utils::stat