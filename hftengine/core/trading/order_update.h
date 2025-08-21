/*
 * Copyright (c) 2025 arvindkrv@protonmail.com
 *
 * Please see the LICENSE file for the terms and conditions
 * associated with this software.
 */

#pragma once

#include <optional>

#include "../trading/order.h"
#include "../types/enums/book_side.h"
#include "../types/enums/order_event_type.h"
#include "../types/enums/order_type.h"
#include "../types/enums/time_in_force.h"
#include "../types/aliases/usings.h"

namespace core::trading {
struct OrderUpdate {
    Timestamp exch_timestamp_;
    Timestamp local_timestamp_;
    int asset_id_;
    OrderId orderId_;
    OrderEventType event_type_;
    std::optional<core::trading::Order> order_;
};
}