/*
 * File: hftengine/core/data/readers/trade_stream_reader.h
 * Description: Class to stream trade data from tardis.
 * Author: Arvind Rathnashyam
 * Date: 2025-06-25
 * License: Proprietary
 */

#pragma once

# include <string>
# include <vector>
# include <unordered_map>
# include <memory>

# include "../../market_data/trade.h"
# include "../../types/usings.h"
# include "../../orderbook/orderbook.h"

class TradeStreamReader {
public:
    explicit TradeStreamReader(double tick_size, double lot_size);
    ~TradeStreamReader();

    // Open CSV file and prepare for reading
    void open(const std::string& filename);

    // Parse next line (returns false on EOF)
    bool parse_next(Trade& trade);

    // Conversion utilities
    PriceTick to_tick(double price) const;
    QuantityLot to_lots(double qty) const;

private:
    double tick_size_;
    double lot_size_;

    // CSV parser implementation
    struct CSVReaderImpl;
    std::unique_ptr<CSVReaderImpl> csv_reader_;
};