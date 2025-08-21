/*
 * File: hftengine/core/market_data/readers/bace_stream_reader.h
 * Description: Class for parsing and reading csv files with market data.
 * Author: Arvind Rathnashyam
 * Date: 2025-06-24
 * License: Proprietary
 */

#pragma once

#include <memory>
#include <string>
#include <unordered_map>
#include <vector>

#include "../../types/aliases/usings.h"
#include "../../../../external/csv/csv.h"

namespace core::market_data {
class BaseStreamReader {
  protected:
    struct CSVReaderImpl {
        io::CSVReader<6> reader;
        std::unordered_map<std::string, size_t> column_map;
        explicit CSVReaderImpl(const std::string &filename);
    };
    void init_csv_reader(const std::string &filename,
                         const std::vector<std::string> &cols);
    std::unique_ptr<CSVReaderImpl> csv_reader_;
    bool has_local_timestamp_ = false;
    Microseconds market_feed_latency_us_ = 0;

  public:
    virtual ~BaseStreamReader() = default;
    virtual void open(const std::string &filename) = 0;
    void set_market_feed_latency_us(Microseconds latency);
};
} // namespace core::market_data