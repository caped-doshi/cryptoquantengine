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
#include "../../market_data/book_update.h"
#include "../../types/enums/book_side.h"
#include "../../types/aliases/usings.h"
#include "book_stream_reader.h"

namespace core::market_data {
BookStreamReader::BookStreamReader() = default;
/*
 * @brief Constructs a BookStreamReader and opens the specified CSV file.
 */
BookStreamReader::BookStreamReader(const std::string &filename) {
    open(filename);
}

void BookStreamReader::open(const std::string &filename) {
    std::vector<std::string> cols = {"timestamp",   "local_timestamp",
                                     "is_snapshot", "side",
                                     "price",       "amount"};
    init_csv_reader(filename, cols);
}
/*
 * @brief Parses the next row from the CSV file and populates the BookUpdate
 * object.
 */
bool BookStreamReader::parse_next(core::market_data::BookUpdate &update) {
    if (!csv_reader_) return false;
    try {
        Timestamp exch_timestamp = 0;
        Timestamp local_timestamp = 0;
        std::string update_type_str;
        std::string side_str;
        double price = 0;
        double quantity = 0;
        if (!csv_reader_->reader.read_row(exch_timestamp, local_timestamp,
                                          update_type_str, side_str, price,
                                          quantity)) {
            return false; 
        }
        if (!has_local_timestamp_) {
            local_timestamp = exch_timestamp + market_feed_latency_us_;
        }
        if (update_type_str.empty() || side_str.empty()) {
            std::cerr << "Warning: Skipped row with missing required fields\n";
            return parse_next(update); // Try next row
        }
        update.exch_timestamp_ = exch_timestamp;
        update.local_timestamp_ = local_timestamp;
        update.update_type_ = (update_type_str == "true")
                                  ? UpdateType::Snapshot
                                  : UpdateType::Incremental;
        update.side_ = (side_str == "bid") ? BookSide::Bid : BookSide::Ask;
        update.price_ = price;
        update.quantity_ = quantity;
        return true;
    } catch (const std::exception &e) {
        std::cerr << "Parsing error: " << e.what() << "\n";
    }
    return false;
}
} 