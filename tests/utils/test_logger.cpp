/*
 * File: tests/utils/test_logger.cpp
 * Description: Unit tests for the thread-safe Logger class.
 * Author: Arvind Rathnashyam
 * Date: 2025-08-14
 * License: Proprietary
 */

#include <catch2/catch_approx.hpp>
#include <catch2/catch_test_macros.hpp>
#include <fstream>
#include <string>
#include <thread>
#include <vector>

#include "utils/logger/logger.h"
#include "utils/logger/log_level.h"

TEST_CASE("[Logger] - single-threaded logging", "[logger][single]") {
    const std::string log_file = "test_logger_single.log";
    {
        Logger logger(log_file, LogLevel::Debug);
        logger.log("First log message", LogLevel::Info);
        logger.log("Second log message", LogLevel::Info);
        logger.flush();
    }

    std::ifstream infile(log_file);
    REQUIRE(infile.is_open());
    std::string line;
    std::vector<std::string> lines;
    while (std::getline(infile, line)) {
        lines.push_back(line);
    }
    infile.close();

    REQUIRE(lines.size() == 2);
    REQUIRE(lines[0] == "First log message");
    REQUIRE(lines[1] == "Second log message");

    std::remove(log_file.c_str());
}

TEST_CASE("[Logger] - multi-threaded logging", "[logger][multi]") {
    const std::string log_file = "test_logger_multi.log";
    {
        Logger logger(log_file, LogLevel::Debug);
        auto log_func = [&logger](int id) {
            for (int i = 0; i < 10; ++i) {
                logger.log("Thread " + std::to_string(id) + " message " +
                        std::to_string(i), LogLevel::Info);
            }
        };

        std::thread t1(log_func, 1);
        std::thread t2(log_func, 2);
        t1.join();
        t2.join();
        logger.flush();
    }

    std::ifstream infile(log_file);
    REQUIRE(infile.is_open());
    std::string line;
    std::vector<std::string> lines;
    while (std::getline(infile, line)) {
        lines.push_back(line);
    }
    infile.close();

    // 20 messages expected (10 from each thread)
    REQUIRE(lines.size() == 20);

    // Optionally, check that all expected messages are present
    int thread1_count = 0, thread2_count = 0;
    for (const auto &l : lines) {
        if (l.find("Thread 1") != std::string::npos) ++thread1_count;
        if (l.find("Thread 2") != std::string::npos) ++thread2_count;
    }
    REQUIRE(thread1_count == 10);
    REQUIRE(thread2_count == 10);

    std::remove(log_file.c_str());
}

TEST_CASE("[Logger] - log level filtering", "[logger][level]") {
    const std::string log_file = "test_logger_levels.log";
    // Create logger with Info level
    auto logger = std::make_shared<Logger>(log_file, LogLevel::Info);

    logger->log("Debug message", LogLevel::Debug);
    logger->log("Info message", LogLevel::Info);
    logger->log("Warning message", LogLevel::Warning);
    logger->log("Error message", LogLevel::Error);

    logger->flush();

    // Read log file
    std::ifstream f(log_file);
    std::string contents((std::istreambuf_iterator<char>(f)),
                        std::istreambuf_iterator<char>());
    f.close();

    // Only Info, Warning, and Error should be present
    REQUIRE(contents.find("Debug message") == std::string::npos);
    REQUIRE(contents.find("Info message") != std::string::npos);
    REQUIRE(contents.find("Warning message") != std::string::npos);
    REQUIRE(contents.find("Error message") != std::string::npos);

    std::remove(log_file.c_str());
}

TEST_CASE("[Logger] - set_level changes filtering", "[logger][level]") {
    const std::string log_file = "test_logger_setlevel.log";
    auto logger = std::make_shared<Logger>(log_file, LogLevel::Error);

    logger->log("Info message", LogLevel::Info);
    logger->log("Error message", LogLevel::Error);

    logger->set_level(LogLevel::Debug);
    logger->log("Debug message", LogLevel::Debug);

    logger->flush();

    std::ifstream f(log_file);
    std::string contents((std::istreambuf_iterator<char>(f)),
                        std::istreambuf_iterator<char>());
    f.close();

    // Only Error and Debug (after set_level) should be present
    REQUIRE(contents.find("Info message") == std::string::npos);
    REQUIRE(contents.find("Error message") != std::string::npos);
    REQUIRE(contents.find("Debug message") != std::string::npos);

    std::remove(log_file.c_str());
}