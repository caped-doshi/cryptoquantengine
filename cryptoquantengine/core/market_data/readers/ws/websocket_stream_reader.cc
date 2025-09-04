/*
 * Copyright (c) 2025 arvindkrv@protonmail.com
 *
 * Please see the LICENSE file for the terms and conditions
 * associated with this software.
 */

#include "websocket_stream_reader.h"
#include <iostream>
#include <websocketpp/client.hpp>
#include <websocketpp/common/thread.hpp>
#include <websocketpp/config/asio_client.hpp>

namespace core::market_data {

BaseWebSocketStreamReader::BaseWebSocketStreamReader() = default;
BaseWebSocketStreamReader::~BaseWebSocketStreamReader() = default;

/*
 * @brief Opens a WebSocket connection to the specified URI.
 */
void BaseWebSocketStreamReader::open(const std::string &uri) {
    std::cout << "[WebSocketStreamReader] Opening WebSocket connection to: "
              << uri << std::endl;
    uri_ = uri;
    connect(uri_);
}

/*
 * @brief Connects to the WebSocket server and sets up handlers.
 * 
 */ 
void BaseWebSocketStreamReader::connect(const std::string &uri) {
    ws_client_ = std::make_unique<client_t>();
    ws_client_->init_asio();

    ws_client_->set_ping_handler(
        [this](websocketpp::connection_hdl hdl, std::string payload) {
            ws_client_->pong(hdl, payload);
            return true;
        });

    ws_client_->set_open_handler([this](websocketpp::connection_hdl hdl) {
        ws_hdl_ = hdl;
        connected_ = true;
        running_ = true;
    });

    ws_client_->set_message_handler(
        [this](websocketpp::connection_hdl, message_ptr msg) {
            std::lock_guard<std::mutex> lock(queue_mutex_);
            message_queue_.push(msg->get_payload());
            queue_cv_.notify_one();
        });

    ws_client_->set_close_handler([this](websocketpp::connection_hdl) {
        connected_ = false;
        running_ = false;
        queue_cv_.notify_all();
    });

    ws_client_->set_tls_init_handler([](websocketpp::connection_hdl) {
        return std::make_shared<websocketpp::lib::asio::ssl::context>(
            websocketpp::lib::asio::ssl::context::sslv23);
    });

    websocketpp::lib::error_code ec;
    auto con = ws_client_->get_connection(uri, ec);
    if (ec) {
        std::cerr << "WebSocket connection error: " << ec.message()
                  << std::endl;
        return;
    }
    ws_client_->connect(con);
    ws_thread_ = std::thread([this] { ws_client_->run(); });
    processing_thread_ =
        std::thread(&BaseWebSocketStreamReader::process_queue, this);
}

void BaseWebSocketStreamReader::disconnect() {
    if (connected_ && ws_client_) {
        ws_client_->close(ws_hdl_, websocketpp::close::status::normal, "");
        connected_ = false;
        running_ = false;
        if (ws_thread_.joinable()) ws_thread_.join();
        if (processing_thread_.joinable()) processing_thread_.join();
        if (rest_thread_.joinable()) rest_thread_.join();
    }
}

void BaseWebSocketStreamReader::process_queue() {
    try {
        while (running_) {
            std::string msg;
            {
                std::unique_lock<std::mutex> lock(queue_mutex_);
                queue_cv_.wait(lock, [this] {
                    return !message_queue_.empty() || !running_;
                });
                if (!running_) break;
                msg = message_queue_.front();
                message_queue_.pop();
            }
            if (!msg.empty()) {
                on_message(msg);
            }
        }
    } catch (const std::exception &e) {
        std::cerr << "[WebSocketStreamReader] Exception in process_queue: " << e.what()
                  << std::endl;
    } catch (...) {
        std::cerr << "[WebSocketStreamReader] Unknown exception in process_queue"
                  << std::endl;
    }
}
bool BaseWebSocketStreamReader::is_connected() const { return connected_; }
} // namespace core::market_data