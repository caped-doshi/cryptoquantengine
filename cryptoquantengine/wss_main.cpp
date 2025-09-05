/*
 * Copyright (c) 2025 arvindkrv@protonmail.com
 *
 * Please see the LICENSE file for the terms and conditions
 * associated with this software.
 */

#include <iostream>
#include <csignal>
#include <atomic>
#include <thread>
#include "core/market_data/readers/ws/binance_stream_reader.h"

std::atomic<bool> running{true};
void signal_handler(int) { running = false; }

int main(int argc, char *argv[]) {
    const std::string symbol = (argc > 1) ? argv[1] : "xrpusdc";
    const std::string book_csv = (argc > 2) ? argv[2] : "xrpusdc_book.csv";
    const std::string trade_csv = (argc > 3) ? argv[3] : "xrpusdc_trade.csv";

    const std::string ws_uri =
        "wss://fstream.binance.com/stream?streams=" + symbol + "@depth@0ms/" +
        symbol + "@trade";
    const std::string rest_uri =
        "https://fapi.binance.com/fapi/v1/depth?symbol=" +
        symbol + "&limit=1000";

    std::signal(SIGINT, signal_handler);

    core::market_data::BinanceStreamReader reader(ws_uri, rest_uri,book_csv, trade_csv);

    std::cout << "Listening to Binance stream for symbol: " << symbol
              << std::endl;
    std::cout << "Book CSV: " << book_csv << "\nTrade CSV: " << trade_csv
              << std::endl;

    while (running) {
        std::this_thread::sleep_for(std::chrono::seconds(1));
    }

    std::cout << "Shutting down Binance stream reader..." << std::endl;
    return 0;
}