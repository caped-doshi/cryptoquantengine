/*
 * File: hftengine/core/market_data/readers/trade_stream_reader.h
 * Description: Class to read tardis trades from a csv.
 * Author: Arvind Rathnashyam
 * Date: 2025-06-24
 * License: Proprietary
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
        Timestamp exch_timestamp;
        Timestamp local_timestamp;
        OrderId orderId;
        std::string side_str;
        double price;
        double quantity;
        if (!csv_reader_->reader.read_row(exch_timestamp, local_timestamp,
                                          orderId, side_str, price, quantity)) {
            return false;
        }
        if (side_str.empty()) {
            std::cerr << "Warning: Skipped row with missing required fields\n";
            return parse_next(trade);
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