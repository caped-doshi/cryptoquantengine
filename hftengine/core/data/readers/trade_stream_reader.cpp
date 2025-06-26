/*
 * File: hft_bt_engine/core/data/readers/trade_stream_reader.h
 * Description: Class to read tardis trades from a csv.
 * Author: Arvind Rathnashyam
 * Date: 2025-06-24
 * License: Proprietary
 */

# include <fstream>
# include <sstream>
# include <string>
# include <iostream>
# include <cmath>
# include <cstdint>

# include "trade_stream_reader.hpp"
# include "../../market_data/trade.h"
# include "../../types/usings.h"
# include "../../types/trade_side.h"
# include "../../../../external/csv/csv.h"


struct TradeStreamReader::CSVReaderImpl {
    io::CSVReader<6> reader;
    std::unordered_map<std::string, size_t> column_map;

    explicit CSVReaderImpl(const std::string& filename)
        : reader(filename) {}  // Initialize CSVReader here
};

TradeStreamReader::~TradeStreamReader() = default;

TradeStreamReader::TradeStreamReader(double tick_size, double lot_size)
    : tick_size_(tick_size), lot_size_(lot_size)
{
    if (tick_size <= 0.0) throw std::invalid_argument("Tick size must be positive");
    if (lot_size <= 0.0) throw std::invalid_argument("Lot size must be positive");
}

void TradeStreamReader::open(const std::string& filename) {
    csv_reader_ = std::make_unique<CSVReaderImpl>(filename);  // Now works

    // Column header processing
    const char* cols[] = { "timestamp", "local_timestamp", "id", 
        "side", "price", "amount" };
    csv_reader_->reader.read_header(io::ignore_extra_column,
        cols[0], cols[1], cols[2],
        cols[3], cols[4], cols[5]);

    // Build column index map
    for (size_t i = 0; i < 6; ++i) {
        if (csv_reader_->reader.has_column(cols[i])) {
            csv_reader_->column_map[cols[i]] = i;
        }
    }
}
bool TradeStreamReader::parse_next(Trade& trade) {
    if (!csv_reader_) {
        return false;
    }

    try {
        // Temporary variables for parsing
        Timestamp timestamp = 1;
        Timestamp local_timestamp = 1;
        OrderId orderId = 1;
        std::string side_str;
        double price = 0;
        double quantity = 0;

        // Read the next row
        if (!csv_reader_->reader.read_row(timestamp, local_timestamp, 
            orderId, side_str, price, quantity)) {
            return false;  // EOF or read error
        }

        // Validate required fields
        if (side_str.empty()) {
            std::cerr << "Warning: Skipped row with missing required fields\n";
            return parse_next(trade);  // Try next row
        }

        // Convert and populate the update
        trade.timestamp_ = timestamp;
        trade.local_timestamp_ = local_timestamp;
        trade.orderId_ = orderId;
        trade.side_ = (side_str == "buy")
            ? TradeSide::Buy
            : TradeSide::Sell;
        trade.price_ = price;
        trade.quantity_ = quantity;

        return true;

    }
    catch (const std::exception& e) {
        std::cerr << "Parsing error: " << e.what() << "\n";
    }

    return false;
}

PriceTick TradeStreamReader::to_tick(double price) const {
    return static_cast<PriceTick>(std::round(price / tick_size_));
}

QuantityLot TradeStreamReader::to_lots(double qty) const {
    return static_cast<QuantityLot>(std::round(qty / lot_size_));
}