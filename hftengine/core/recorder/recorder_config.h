/*
 * Copyright (c) 2025 arvindkrv@protonmail.com
 *
 * Please see the LICENSE file for the terms and conditions
 * associated with this software.
 */

#pragma once
#include "../../utils/logger/log_level.h"
#include <string>

struct RecorderConfig {
    std::uint64_t interval_us = 1'000'000; // Recording interval in microseconds
    std::string output_file = "recorder_output.csv";
};