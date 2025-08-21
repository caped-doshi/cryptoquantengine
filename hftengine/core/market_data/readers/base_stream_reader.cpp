/*
 * File: hftengine/core/market_data/readers/bace_stream_reader.cpp
 * Description: Class for parsing and reading csv files with market data.
 * Author: Arvind Rathnashyam
 * Date: 2025-08-21
 * License: Proprietary
 */

#include "base_stream_reader.h"
#include "../../../../external/csv/csv.h"

namespace core::market_data {
BaseStreamReader::CSVReaderImpl::CSVReaderImpl(const std::string &filename)
    : reader(filename) {}

void BaseStreamReader::init_csv_reader(const std::string &filename,
                                       const std::vector<std::string> &cols) {
    csv_reader_ = std::make_unique<CSVReaderImpl>(filename);
    csv_reader_->reader.read_header(
        io::ignore_extra_column | io::ignore_missing_column, cols[0].c_str(),
        cols[1].c_str(), cols[2].c_str(), cols[3].c_str(), cols[4].c_str(),
        cols[5].c_str());
    for (size_t i = 0; i < cols.size(); ++i) {
        if (csv_reader_->reader.has_column(cols[i])) {
            csv_reader_->column_map[cols[i]] = i;
        }
    }
    has_local_timestamp_ = csv_reader_->reader.has_column("local_timestamp");
}

void BaseStreamReader::set_market_feed_latency_us(Microseconds latency) {
    market_feed_latency_us_ = latency;
}
} // namespace core::market_data