/*
 * Copyright (c) 2025 arvindkrv@protonmail.com
 *
 * Please see the LICENSE file for the terms and conditions
 * associated with this software.
 */

#pragma once

#include "../types/enums/book_side.h"
#include "../types/enums/trade_side.h"
#include "../types/enums/order_type.h"
#include "../types/enums/time_in_force.h"
#include "../types/aliases/usings.h"

namespace core::trading {
struct Fill {
    int asset_id_;
    Timestamp exch_timestamp_;  
    Timestamp local_timestamp_; 
    OrderId orderId_;
    TradeSide side_;
    Price price_;
    Quantity quantity_;
    bool is_maker;
};
}