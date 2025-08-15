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

TEST_CASE("[Logger] - single-threaded logging", "[logger][single]") {
     const std::string log_file = "test_logger_single.log";
     {
         Logger logger(log_file);
         logger.log("First log message");
         logger.log("Second log message");
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
         Logger logger(log_file);
         auto log_func = [&logger](int id) {
             for (int i = 0; i < 10; ++i) {
                 logger.log("Thread " + std::to_string(id) + " message " +
                            std::to_string(i));
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