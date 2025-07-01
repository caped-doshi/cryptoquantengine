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
    /*
    std::string exchange_;       
    std::string exchangeId_;   
    std::string symbol;     */  // available in full data    

    Timestamp timestamp_;         
    Timestamp localTimestamp_;   

    UpdateType update_type_;     // Snapshot or Incremental
    BookSide side_;                  // Bid or Ask

    Price price_;               // price identifying book level being updated
    Quantity quantity_;         // updated price level amount as provided by exchange, not a delta (0 = delete)
};