/*
 * File: hftengine/utils/logger/logger.h
 * Description: Thread-safe logger that writes log messages on a separate
 * thread. 
 * Author: Arvind Rathnashyam 
 * Date: 2025-08-14 
 * License: Proprietary
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

class Logger {
  public:
    explicit Logger(const std::string &filename, LogLevel level);
    ~Logger();
    void log(const std::string &message, LogLevel level);
    void flush();
    void set_level(LogLevel level);

  private:
    void process(); // Logging thread function

    std::queue<std::string> messages_;
    std::mutex mutex_;
    std::condition_variable cond_var_;
    std::thread logging_thread_;
    std::ofstream log_file_;
    std::atomic<bool> exit_flag_;
    std::atomic<LogLevel> log_level_;
};