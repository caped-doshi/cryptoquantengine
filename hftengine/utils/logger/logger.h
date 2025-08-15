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

class Logger {
  public:
    // Construct logger with output file
    explicit Logger(const std::string &filename);
    // Destructor ensures all messages are flushed and thread is joined
    ~Logger();
    // Thread-safe log method
    void log(const std::string &message);
    // Optionally, flush remaining messages immediately
    void flush();

  private:
    void process(); // Logging thread function

    std::queue<std::string> messages_;
    std::mutex mutex_;
    std::condition_variable cond_var_;
    std::thread logging_thread_;
    std::ofstream log_file_;
    std::atomic<bool> exit_flag_;
};