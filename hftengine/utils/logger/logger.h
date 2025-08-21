/*
 * Copyright (c) 2025 Arvind Rathnashyam - arvindkrv@protonmail.com
 *
 * Please see the LICENSE file for the terms and conditions
 * associated with this software.
 */

#pragma once

#include <atomic>
#include <condition_variable>
#include <fstream>
#include <mutex>
#include <queue>
#include <string>
#include <thread>

#include "log_level.h"

namespace utils::logger {
class Logger {
  public:
    explicit Logger(const std::string &filename, utils::logger::LogLevel level);
    ~Logger();
    void log(const std::string &message, utils::logger::LogLevel level);
    void flush();
    void set_level(utils::logger::LogLevel level);

  private:
    void process(); 

    std::queue<std::string> messages_;
    std::mutex mutex_;
    std::condition_variable cond_var_;
    std::thread logging_thread_;
    std::ofstream log_file_;
    std::atomic<bool> exit_flag_;
    std::atomic<LogLevel> log_level_;
};
} // namespace utils::logger