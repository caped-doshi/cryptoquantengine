/*
 * File: hftengine/core/readers/trade_stream_reader.h
 * Description: Class to stream trade data from tardis.
 * Author: Arvind Rathnashyam
 * Date: 2025-06-25
 * License: Proprietary
 */

#pragma once

#include <memory>
#include <string>
#include <unordered_map>
#include <vector>

#include "../../../../external/csv/csv.h"
#include "../../orderbook/orderbook.h"
#include "../../types/usings.h"
#include "../trade.h"

namespace core::market_data {
class TradeStreamReader {
  public:
    TradeStreamReader();
    explicit TradeStreamReader(const std::string &filename);

    void open(const std::string &filename);
    bool parse_next(core::market_data::Trade &trade);

  private:
    struct CSVReaderImpl {
        io::CSVReader<6> reader;
        std::unordered_map<std::string, size_t> column_map;

        explicit CSVReaderImpl(const std::string &filename)
            : reader(filename) {}
    };
    std::unique_ptr<CSVReaderImpl> csv_reader_;
};
} // namespace core::market_data