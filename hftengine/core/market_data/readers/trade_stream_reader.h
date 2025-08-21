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
#include "../../types/aliases/usings.h"
#include "../trade.h"
#include "base_stream_reader.h"

namespace core::market_data {
class TradeStreamReader : public BaseStreamReader {
  public:
    TradeStreamReader();
    explicit TradeStreamReader(const std::string &filename);

    void open(const std::string &filename) override;
    bool parse_next(core::market_data::Trade &trade);
};
} // namespace core::market_data