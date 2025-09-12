/*
 * Copyright (c) 2025 arvindkrv@protonmail.com
 *
 * Please see the LICENSE file for the terms and conditions
 * associated with this software.
 */

#pragma once

#include "../../../types/aliases/usings.h"
#include "../../book_update.h"
#include "../../trade.h"
#include "websocket_stream_reader.h"
#include <chrono>
#include <fstream>
#include <json/json.hpp>
#include <memory>
#include <mutex>
#include <queue>
#include <string>

namespace core::market_data {

class BinanceStreamReader : public BaseWebSocketStreamReader {
  public:
    BinanceStreamReader();
    ~BinanceStreamReader() override;
    explicit BinanceStreamReader(const std::string &ws_uri,
                                 const std::string &rest_uri,
                                 const std::string &book_csv,
                                 const std::string &trade_csv,
                                 bool enable_csv_writer);

    void open(const std::string &uri) override;

    bool parse_next_book(BookUpdate &update);
    bool parse_next_trade(Trade &trade);

  protected:
    void on_message(const std::string &msg) override;

  private:
    std::queue<BookUpdate> book_queue_;
    std::queue<Trade> trade_queue_;
    std::mutex queue_mutex_;
    std::mutex write_queue_mutex_;
    std::condition_variable book_cv_, trade_cv_;
    std::thread csv_writer_thread_;
    std::atomic<bool> running_{false};
    bool enable_csv_writer_ = false;

    std::ofstream book_csv_;
    std::ofstream trade_csv_;
    bool book_header_written_ = false;
    bool trade_header_written_ = false;

    void handle_book_message(const nlohmann::json &j);
    void handle_trade_message(const nlohmann::json &j);
    void poll_rest_snapshots(const std::string &rest_uri);
    void csv_write_loop();
};

} // namespace core::market_data