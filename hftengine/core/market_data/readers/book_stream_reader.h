/*
 * Copyright (c) 2025 arvindkrv@protonmail.com
 *
 * Please see the LICENSE file for the terms and conditions
 * associated with this software.
 */

#pragma once

#include <memory>
#include <string>
#include <unordered_map>
#include <vector>

#include "../../../../external/csv/csv.h"
#include "../../market_data/book_update.h"
#include "../../types/enums/book_side.h"
#include "../../types/aliases/usings.h"
#include "base_stream_reader.h"

namespace core::market_data {
class BookStreamReader : public BaseStreamReader {
  public:
    BookStreamReader();
    explicit BookStreamReader(const std::string &filename);

    void open(const std::string &filename) override;
    bool parse_next(core::market_data::BookUpdate &update);
};
}