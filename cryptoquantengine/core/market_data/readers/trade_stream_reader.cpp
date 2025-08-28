/*
 * Copyright (c) 2025 arvindkrv@protonmail.com
 *
 * Please see the LICENSE file for the terms and conditions
 * associated with this software.
 */

#include <cmath>
#include <cstdint>
#include <fstream>
#include <iostream>
#include <sstream>
#include <string>

#include "../../../../external/csv/csv.h"
#include "../../market_data/trade.h"
#include "../../types/enums/trade_side.h"
#include "../../types/aliases/usings.h"
#include "trade_stream_reader.h"

namespace core::market_data {

TradeStreamReader::TradeStreamReader() = default;

TradeStreamReader::TradeStreamReader(const std::string &filename) {
    open(filename);
}

void TradeStreamReader::open(const std::string &filename) {
    std::vector<std::string> cols = {"timestamp", "local_timestamp", "id",
                                     "side",      "price",           "amount"};
    init_csv_reader(filename, cols);
}
bool TradeStreamReader::parse_next(core::market_data::Trade &trade) {
    if (!csv_reader_) return false;
    try {
        Timestamp exch_timestamp = 0;
        Timestamp local_timestamp = 0;
        OrderId orderId = 0;
        std::string side_str;
        Price price = 0.0;
        Quantity quantity = 0.0;
        if (!csv_reader_->reader.read_row(exch_timestamp, local_timestamp,
                                          orderId, side_str, price, quantity)) {
            return false;
        }
        if (side_str.empty()) {
            std::cerr << "Warning: Skipped row with missing required fields\n";
            return parse_next(trade);
        }
        if (!has_local_timestamp_) {
            local_timestamp = exch_timestamp + market_feed_latency_us_;
        }
        trade.exch_timestamp_ = exch_timestamp;
        trade.local_timestamp_ = local_timestamp;
        trade.orderId_ = orderId;
        trade.side_ = (side_str == "buy") ? TradeSide::Buy : TradeSide::Sell;
        trade.price_ = price;
        trade.quantity_ = quantity;
        return true;
    } catch (const std::exception &e) {
        std::cerr << "Parsing error: " << e.what() << "\n";
    }
    return false;
}
} 