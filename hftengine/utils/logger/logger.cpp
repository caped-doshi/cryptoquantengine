/*
 * File: hftengine/utils/logger/logger.cpp
 * Description: Implementation of thread-safe logger.
 * Author: Arvind Rathnashyam
 * Date: 2025-08-14
 * License: Proprietary
 */

#include <atomic>
#include <condition_variable>
#include <fstream>
#include <iostream>
#include <mutex>
#include <queue>
#include <string>
#include <thread>

#include "logger.h"
#include "log_level.h"

namespace utils {
namespace logger {
/*
 * @brief Constructs a Logger that writes to the specified file.
 */
Logger::Logger(const std::string &filename, utils::logger::LogLevel level)
    : exit_flag_(false), log_level_(level) {
    log_file_.open(filename, std::ios::out | std::ios::app);
    logging_thread_ = std::thread(&Logger::process, this);
}
/*
 * @brief Flushes messages and joins the logging thread.
 */
Logger::~Logger() {
    {
        std::lock_guard<std::mutex> lock(mutex_);
        exit_flag_ = true;
    }
    cond_var_.notify_one();
    if (logging_thread_.joinable()) logging_thread_.join();
    log_file_.close();
}
/*
 * @brief Logs a message by adding it to the queue and notifying the logging
 * thread.
 */
void Logger::log(const std::string &message, utils::logger::LogLevel level) {
    if (level < log_level_) return;
    std::lock_guard<std::mutex> lock(mutex_);
    messages_.push(message);
    cond_var_.notify_one();
}
/*
 * @brief Flushes all messages to the log file immediately.
 *
 * This method processes all messages in the queue and writes them to the log
 * file, ensuring that no messages are left unlogged.
 */
void Logger::flush() {
    std::unique_lock<std::mutex> lock(mutex_);
    while (!messages_.empty()) {
        log_file_ << messages_.front() << std::endl;
        messages_.pop();
    }
    log_file_.flush();
}
/*
 * @brief The logging thread function that processes messages from the queue.
 *
 * Listens for new messages and writes them to the log file until the exit.
 */
void Logger::process() {
    while (true) {
        std::unique_lock<std::mutex> lock(mutex_);
        cond_var_.wait(lock,
                       [this] { return !messages_.empty() || exit_flag_; });

        while (!messages_.empty()) {
            log_file_ << messages_.front() << std::endl;
            messages_.pop();
        }
        log_file_.flush();

        if (exit_flag_ && messages_.empty()) break;
    }
}

/*
 * @brief
 */
void Logger::set_level(utils::logger::LogLevel level) { log_level_ = level; }

} // namespace logger
} // namespace utils