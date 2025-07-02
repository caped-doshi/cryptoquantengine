/*
 * File: hft_bt_engine/core/data/readers/book_stream_reader.cpp
 * Description: Class for parsing and reading csv files with tardis incremental
 * book data.
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
#include "../../market_data/book_update.h"
#include "../../types/book_side.h"
#include "../../types/usings.h"
#include "book_stream_reader.h"

BookStreamReader::BookStreamReader() = default;

BookStreamReader::BookStreamReader(const std::string &filename) {
    open(filename);
}

void BookStreamReader::open(const std::string &filename) {
    csv_reader_ = std::make_unique<CSVReaderImpl>(filename); // Now works

    // Column header processing
    const char *cols[] = {"timestamp", "local_timestamp", "is_snapshot",
                          "side",      "price",           "amount"};
    csv_reader_->reader.read_header(io::ignore_extra_column, cols[0], cols[1],
                                    cols[2], cols[3], cols[4], cols[5]);

    // Build column index map
    for (size_t i = 0; i < 6; ++i) {
        if (csv_reader_->reader.has_column(cols[i])) {
            csv_reader_->column_map[cols[i]] = i;
        }
    }
}
bool BookStreamReader::parse_next(BookUpdate &update) {
    if (!csv_reader_) {
        return false;
    }

    try {
        // Temporary variables for parsing
        double timestamp = 0;
        double local_timestamp = 0;
        std::string update_type_str;
        std::string side_str;
        double price = 0;
        double quantity = 0;

        // Read the next row
        if (!csv_reader_->reader.read_row(timestamp, local_timestamp,
                                          update_type_str, side_str, price,
                                          quantity)) {
            return false; // EOF or read error
        }

        // Validate required fields
        if (update_type_str.empty() || side_str.empty()) {
            std::cerr << "Warning: Skipped row with missing required fields\n";
            return parse_next(update); // Try next row
        }

        // Convert and populate the update
        update.timestamp_ = static_cast<Timestamp>(timestamp);
        update.localTimestamp_ = static_cast<Timestamp>(local_timestamp);
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