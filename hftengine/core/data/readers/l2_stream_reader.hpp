/*
 * File: hftengine/core/data/readers/l2_stream_reader.h
 * Description: Class to read L2 data from tardis incremental_book_update csv files.
 * Author: Arvind Rathnashyam
 * Date: 2025-06-24
 * License: Proprietary
 */

#pragma once

# include <string>
# include <vector>
# include <unordered_map>
# include <memory>

# include "../../market_data/l2_update.h"
# include "../../types/usings.h"
# include "../../types/book_side.h"

class L2StreamReader {
public:
    explicit L2StreamReader(double tick_size, double lot_size);
    ~L2StreamReader();

    // Open CSV file and prepare for reading
    void open(const std::string& filename);

    // Parse next line (returns false on EOF)
    bool parse_next(L2Update& update);

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