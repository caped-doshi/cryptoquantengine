/*
 * Copyright (c) 2025 arvindkrv@protonmail.com
 *
 * Please see the LICENSE file for the terms and conditions
 * associated with this software.
 */

#include <curl/curl.h>
#include <iostream>
#include <json/json.hpp>

#include "../../../../utils/http/http_utils.h"
#include "../../../types/enums/update_type.h"
#include "../../book_update.h"
#include "../../trade.h"
#include "binance_stream_reader.h"

namespace core::market_data {

BinanceStreamReader::BinanceStreamReader() = default;

BinanceStreamReader::BinanceStreamReader(const std::string &ws_uri,
                                         const std::string &rest_uri,
                                         const std::string &book_csv,
                                         const std::string &trade_csv) {
    std::cout << "[BinanceStreamReader] Constructor called" << std::endl;
    book_csv_.open(book_csv, std::ios::out | std::ios::app);
    trade_csv_.open(trade_csv, std::ios::out | std::ios::app);
    open(ws_uri);
    running_ = true;
    csv_writer_thread_ = std::thread([this] { csv_write_loop(); });
    rest_thread_ =
        std::thread([this, rest_uri] { poll_rest_snapshots(rest_uri); });
}

BinanceStreamReader::~BinanceStreamReader() {
    std::cout << "[BinanceStreamReader] Destructor called" << std::endl;
    running_ = false;
    book_cv_.notify_all();
    trade_cv_.notify_all();
    if (csv_writer_thread_.joinable()) csv_writer_thread_.join();
    if (book_csv_.is_open()) book_csv_.close();
    if (trade_csv_.is_open()) trade_csv_.close();
}

void BinanceStreamReader::open(const std::string &uri) {
    std::cout << "[BinanceStreamReader] Opening WebSocket connection to: "
              << uri << std::endl;
    BaseWebSocketStreamReader::open(uri);
    std::cout << "[BinanceStreamReader] WebSocket connection opened"
              << std::endl;
    if (book_csv_.is_open() && !book_header_written_) {
        std::cout << "[BinanceStreamReader] Writing book CSV header"
                  << std::endl;
        book_csv_
            << "timestamp,local_timestamp,is_snapshot,side,price,amount\n";
        book_header_written_ = true;
    }
    if (trade_csv_.is_open() && !trade_header_written_) {
        trade_csv_ << "timestamp,local_timestamp,id,side,price,amount\n";
        trade_header_written_ = true;
    }
}

/*
 * 
 */
void BinanceStreamReader::on_message(const std::string &msg) {
    /*{"stream":"btcusdt@depth","data":{"e":"depthUpdate","E":1756875694535,"T":1756875694532,"s":"BTCUSDT","U":8503862928430,"u":8503862940039,"pu":8503862928383,"b":[["1000.00","13.213"],...,["110991.90","23.928"]],"a":[["110992.00","2.988"],...,["116541.40","0.002"]]}}
     */
    try {
        auto j = nlohmann::json::parse(msg);
        if (j.contains("stream") && j.contains("data")) {
            const auto &data = j["data"];
            if (data.contains("e")) {
                std::string event_type = data["e"];
                if (event_type == "depthUpdate") {
                    handle_book_message(data);
                } else if (event_type == "trade") {
                    handle_trade_message(data);
                }
            }
        }
    } catch (const std::exception &e) {
        std::cerr << "[BinanceStreamReader] JSON parse error: " << e.what()
                  << "\n";
    }
}

void BinanceStreamReader::handle_book_message(const nlohmann::json &j) {
    /*{
        "E" : 1756915941690,
        "T" : 1756915941682,
        "U" : 8507427226956,
        "a" : [
            [ "2.8707", "0.0" ],     [ "2.8708", "0.0" ],
            [ "2.8709", "0.0" ],     [ "2.8710", "4076.4" ],
            [ "2.8711", "897.0" ],   [ "2.8712", "5598.6" ],
            [ "2.8713", "20904.5" ], [ "2.8714", "3796.9" ],
            [ "2.8715", "5675.9" ],  [ "2.8716", "7375.4" ],
            [ "2.8717", "7187.6" ],  [ "2.8718", "16050.2" ],
            [ "2.8719", "26235.0" ], [ "2.8720", "26178.6" ],
            [ "2.8723", "10514.9" ], [ "2.8727", "10101.4" ],
            [ "2.8728", "6133.5" ],  [ "2.8729", "26146.1" ],
            [ "2.8730", "9009.8" ],  [ "2.8732", "15791.6" ],
            [ "2.8734", "13629.8" ], [ "2.8735", "4767.3" ],
            [ "2.8737", "20610.7" ], [ "2.8739", "7642.9" ],
            [ "2.8746", "17628.0" ], [ "2.8750", "46142.3" ],
            [ "2.8751", "3234.6" ],  [ "2.8752", "10057.4" ],
            [ "2.8794", "11392.1" ], [ "2.8795", "6996.4" ],
            [ "2.8837", "1500.9" ],  [ "2.8838", "29495.6" ],
            [ "2.8851", "1649.2" ],  [ "2.8967", "1300.0" ],
            [ "2.8997", "1236.8" ],  [ "2.9416", "3293.4" ]
        ],
        "b" : [
            [ "2.2965", "11.5" ],    [ "2.8659", "107934.7" ],
            [ "2.8660", "3521.8" ],  [ "2.8662", "24104.5" ],
            [ "2.8663", "22314.3" ], [ "2.8669", "11629.5" ],
            [ "2.8672", "16907.9" ], [ "2.8680", "16914.7" ],
            [ "2.8683", "15979.8" ], [ "2.8686", "11687.2" ],
            [ "2.8687", "16340.6" ], [ "2.8691", "8706.7" ],
            [ "2.8692", "20963.5" ], [ "2.8694", "15059.2" ],
            [ "2.8696", "7664.0" ],  [ "2.8697", "6714.5" ],
            [ "2.8698", "19954.4" ], [ "2.8699", "19891.6" ],
            [ "2.8700", "19415.8" ], [ "2.8701", "38791.5" ],
            [ "2.8702", "29166.7" ], [ "2.8703", "21780.6" ],
            [ "2.8704", "30687.8" ], [ "2.8705", "25393.6" ],
            [ "2.8706", "22339.9" ], [ "2.8707", "20755.8" ],
            [ "2.8708", "26445.2" ], [ "2.8709", "22285.7" ]
        ],
        "e" : "depthUpdate",
        "pu" : 8507427220056,
        "s" : "XRPUSDC",
        "u" : 8507427267343
    }*/
    BookUpdate update;
    update.exch_timestamp_ = 1000 * j.value("T", static_cast<std::uint64_t>(0));
    update.local_timestamp_ =
        1000 * j.value("E", static_cast<std::uint64_t>(0));
    if (j.contains("b") && !j["b"].empty()) {
        for (const auto &bid : j["b"]) {
            update.side_ = BookSide::Bid;
            update.price_ = std::stod(bid[0].get<std::string>());
            update.quantity_ = std::stod(bid[1].get<std::string>());
            update.update_type_ = UpdateType::Incremental;
            book_queue_.push(update);
            book_cv_.notify_one();
        }
    }
    if (j.contains("a") && !j["a"].empty()) {
        for (const auto &ask : j["a"]) {
            update.side_ = BookSide::Ask;
            update.price_ = std::stod(ask[0].get<std::string>());
            update.quantity_ = std::stod(ask[1].get<std::string>());
            update.update_type_ = UpdateType::Incremental;
            book_queue_.push(update);
            book_cv_.notify_one();
        }
    }
}

void BinanceStreamReader::handle_trade_message(const nlohmann::json &j) {
    /*{
      "E": 1756922507818,
      "T": 1756922507818,
      "X": "MARKET",
      "e": "trade",
      "m": false,
      "p": "2.8667",
      "q": "18.6",
      "s": "XRPUSDC",
      "t": 117581739
    }*/
    Trade trade;
    trade.exch_timestamp_ = 1000 * j.value("T", static_cast<std::uint64_t>(0));
    trade.local_timestamp_ = 1000 * j.value("E", static_cast<std::uint64_t>(0));
    trade.orderId_ = j.value("t", static_cast<std::uint64_t>(0));
    trade.price_ = std::stod(j.value("p", "0"));
    trade.quantity_ = std::stod(j.value("q", "0"));
    trade.side_ = j.value("m", false) ? TradeSide::Buy : TradeSide::Sell;
    trade_queue_.push(trade);
}

bool BinanceStreamReader::parse_next_book(BookUpdate &update) {
    std::lock_guard<std::mutex> lock(queue_mutex_);
    if (book_queue_.empty()) return false;
    update = book_queue_.front();
    book_queue_.pop();
    return true;
}

bool BinanceStreamReader::parse_next_trade(Trade &trade) {
    std::lock_guard<std::mutex> lock(queue_mutex_);
    if (trade_queue_.empty()) return false;
    trade = trade_queue_.front();
    trade_queue_.pop();
    return true;
}

void BinanceStreamReader::poll_rest_snapshots(const std::string &rest_uri) {
    /*
    {"lastUpdateId":8509976781069,"E":1756951185683,"T":1756951185662,"bids":[["2.8401","14252.6"],["2.8400","32721.6"],["2.8399","11071.3"],["2.8398","22734.2"],["2.8397","25936.4"]],"asks":[["2.8402","4860.5"],["2.8403","30948.3"],["2.8404","12429.9"],["2.8405","2258.8"],["2.8406","8144.3"]]}
    */
    while (running_) {
        try {
            std::string response = utils::http::http_get(rest_uri);
            const auto snapshot = nlohmann::json::parse(response);
            Timestamp now =
                std::chrono::duration_cast<std::chrono::microseconds>(
                    std::chrono::system_clock::now().time_since_epoch())
                    .count();
            {
                //std::lock_guard<std::mutex> lock(queue_mutex_);
                if (snapshot.contains("bids")) {
                    for (const auto &bid : snapshot["bids"]) {
                        BookUpdate update;
                        update.exch_timestamp_ =
                            1000 * snapshot.value("T", static_cast<std::uint64_t>(0));
                        update.local_timestamp_ =
                            1000 * snapshot.value("E", static_cast<std::uint64_t>(0));;
                        update.update_type_ = UpdateType::Snapshot;
                        update.side_ = BookSide::Bid;
                        update.price_ = std::stod(bid[0].get<std::string>());
                        update.quantity_ = std::stod(bid[1].get<std::string>());
                        book_queue_.push(update);
                        book_cv_.notify_one();
                    }
                }
                if (snapshot.contains("asks")) {
                    for (const auto &ask : snapshot["asks"]) {
                        BookUpdate update;
                        update.exch_timestamp_ =
                            1000 * snapshot.value("T", static_cast<std::uint64_t>(0));
                        update.local_timestamp_ =
                            1000 * snapshot.value("E", static_cast<std::uint64_t>(0));
                        update.update_type_ = UpdateType::Snapshot;
                        update.side_ = BookSide::Ask;
                        update.price_ = std::stod(ask[0].get<std::string>());
                        update.quantity_ = std::stod(ask[1].get<std::string>());
                        book_queue_.push(update);
                        book_cv_.notify_one();
                    }
                }
            }
            std::cout << "[BinanceStreamReader] Snapshot pushed at " << now
                      << std::endl;

        } catch (const std::exception &e) {
            std::cerr << "[BinanceStreamReader] Snapshot loop error: "
                      << e.what() << std::endl;
        } catch (...) {
            std::cerr << "[BinanceStreamReader] Snapshot loop unknown error"
                      << std::endl;
        }
        for (int i = 0; i < 1; ++i) {
            if (!running_) break;
            std::this_thread::sleep_for(std::chrono::minutes(1));
        }
    }
}

void BinanceStreamReader::csv_write_loop() {
    try {
        std::cout << "[BinanceStreamReader] csv_write_loop started"
                  << std::endl;
        std::unique_lock<std::mutex> lock(write_queue_mutex_);
        while (running_) {
            book_cv_.wait(lock, [this] {
                return !book_queue_.empty() || !trade_queue_.empty() ||
                       !running_;
            });
            while (!book_queue_.empty()) {
                const BookUpdate &update = book_queue_.front();
                if (book_csv_.is_open()) {
                    book_csv_ << update.exch_timestamp_ << ","
                              << update.local_timestamp_ << ","
                              << (update.update_type_ == UpdateType::Snapshot
                                      ? "true"
                                      : "false")
                              << ","
                              << (update.side_ == BookSide::Bid ? "bid" : "ask")
                              << "," << update.price_ << "," << update.quantity_
                              << "\n";
                }
                book_queue_.pop();
            }

            while (!trade_queue_.empty()) {
                const Trade &trade = trade_queue_.front();
                if (trade_csv_.is_open()) {
                    trade_csv_
                        << trade.exch_timestamp_ << ","
                        << trade.local_timestamp_ << "," << trade.orderId_
                        << ","
                        << (trade.side_ == TradeSide::Buy ? "buy" : "sell")
                        << "," << trade.price_ << "," << trade.quantity_
                        << "\n";
                }
                trade_queue_.pop();
            }
        }
    } catch (const std::exception &e) {
        std::cerr << "[BinanceStreamReader] CSV write loop error: " << e.what()
                  << std::endl;
    } catch (...) {
        std::cerr << "[BinanceStreamReader] CSV write loop unknown error"
                  << std::endl;
    }
}
} // namespace core::market_data