/*
 * File: hftengine/core/market_data/readers/book_stream_reader.h
 * Description: Class to read L2 data from tardis incremental_book_update csv
 * files.
 * Author: Arvind Rathnashyam
 * Date: 2025-06-24
 * License: Proprietary
 */

#pragma once

#include <memory>
#include <string>
#include <unordered_map>
#include <vector>

#include "../../../../external/csv/csv.h"
#include "../../market_data/book_update.h"
#include "../../types/book_side.h"
#include "../../types/usings.h"

namespace core::market_data {
class BookStreamReader {
  public:
    BookStreamReader();
    explicit BookStreamReader(const std::string &filename);

    void open(const std::string &filename);
    bool parse_next(core::market_data::BookUpdate &update);

  private:
    struct CSVReaderImpl {
        io::CSVReader<6> reader;
        std::unordered_map<std::string, size_t> column_map;

        explicit CSVReaderImpl(const std::string &filename)
            : reader(filename) {}
    };
    std::unique_ptr<CSVReaderImpl> csv_reader_;
    bool has_local_timestamp_ = false;
};
}