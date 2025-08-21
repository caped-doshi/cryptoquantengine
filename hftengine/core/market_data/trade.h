/*
 * Copyright (c) 2025 arvindkrv@protonmail.com
 *
 * Please see the LICENSE file for the terms and conditions
 * associated with this software.
 */

#pragma once

# include "../types/aliases/usings.h"
# include "../types/enums/trade_side.h"

namespace core::market_data {
struct Trade {
    Timestamp exch_timestamp_;  // arrives at exchange first
    Timestamp local_timestamp_; // then sent to local with latency
    TradeSide side_;
    Price price_;
    Quantity quantity_;
    OrderId orderId_;
};
} 