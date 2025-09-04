/*
 * Copyright (c) 2025 arvindkrv@protonmail.com
 *
 * Please see the LICENSE file for the terms and conditions
 * associated with this software.
 */

#pragma once

#include <atomic>
#include <condition_variable>
#include <memory>
#include <mutex>
#include <queue>
#include <string>
#include <thread>
#include <websocketpp/client.hpp>
#include <websocketpp/config/asio_client.hpp>

namespace core::market_data {

class BaseWebSocketStreamReader {
  public:
    using client_t = websocketpp::client<websocketpp::config::asio_tls_client>;
    using message_ptr = websocketpp::config::asio_client::message_type::ptr;

    BaseWebSocketStreamReader();
    virtual ~BaseWebSocketStreamReader();

    virtual void open(const std::string &uri);
    bool is_connected() const;

  protected:
    std::string uri_;
    std::atomic<bool> connected_;
    std::atomic<bool> running_;

    std::queue<std::string> message_queue_;
    std::mutex queue_mutex_;
    std::condition_variable queue_cv_;
    std::thread ws_thread_;
    std::thread processing_thread_;
    std::thread rest_thread_;

    std::unique_ptr<client_t> ws_client_;
    websocketpp::connection_hdl ws_hdl_;

    void connect(const std::string &uri);
    void disconnect();

    virtual void on_message(const std::string &msg) = 0;
    void process_queue();
};

} // namespace core::market_data