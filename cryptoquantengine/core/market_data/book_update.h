/*
 * Copyright (c) 2025 arvindkrv@protonmail.com
 *
 * Please see the LICENSE file for the terms and conditions
 * associated with this software.
 */

# pragma once

#include <string>

#include "../types/aliases/usings.h" 
#include "../types/enums/update_type.h"
#include "../types/enums/book_side.h"

namespace core::market_data {
struct BookUpdate {
    Timestamp exch_timestamp_;  // arrives at exchange first
    Timestamp local_timestamp_; // sent to local with latency

    UpdateType update_type_; // Snapshot or Incremental
    BookSide side_;

    Price price_;
    Quantity quantity_; // updated price level amount as provided by exchange,
                        // not a delta (0 = delete)
};
}