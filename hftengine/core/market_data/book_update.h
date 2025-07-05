/*
 * File: hft_bt_engine/core/market_data/BookUpdate.h
 * Description: Class defining a book update.
 * Author: Arvind Rathnashyam
 * Date: 2025-06-24
 * License: Proprietary
 */

# pragma once

#include <string>

#include "../types/usings.h" 
#include "../types/update_type.h"
#include "../types/book_side.h"

struct BookUpdate {
    Timestamp exch_timestamp_;   // arrives at exchange first 
    Timestamp local_timestamp_;  // sent to local with latency

    UpdateType update_type_;     // Snapshot or Incremental
    BookSide side_;

    Price price_;               
    Quantity quantity_;          // updated price level amount as provided by exchange, not a delta (0 = delete)
};