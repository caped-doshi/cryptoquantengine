/*
 * Copyright (c) 2025 arvindkrv@protonmail.com
 *
 * Please see the LICENSE file for the terms and conditions
 * associated with this software.
 */

#pragma once

#include "../types/enums/book_side.h"
#include "../types/enums/order_status.h"
#include "../types/enums/order_type.h"
#include "../types/enums/time_in_force.h"
#include "../types/aliases/usings.h"

namespace core::trading {
struct Order {
    Timestamp local_timestamp_;
    Timestamp exch_timestamp_;
    OrderId orderId_;
    BookSide side_;
    Price price_;
    Quantity quantity_;
    Quantity filled_quantity_;
    TimeInForce tif_;
    OrderType orderType_;
    Quantity queueEst_;
    OrderStatus orderStatus_;
};
} 