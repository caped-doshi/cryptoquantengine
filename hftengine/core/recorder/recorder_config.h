/*
 * File: hftengine/core/recorder/recorder_config.h
 * Description: Configuration struct for the Recorder component.
 * Author: Arvind Rathnashyam
 * Date: 2025-08-19
 * License: Proprietary
 */

#pragma once
#include "../../utils/logger/log_level.h"
#include <string>

struct RecorderConfig {
    std::uint64_t interval_us = 1'000'000; // Recording interval in microseconds
    std::string output_file = "recorder_output.csv";
};