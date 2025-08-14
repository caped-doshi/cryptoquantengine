/*
 * File: hftengine/core/trading/order.h
 * Description: Defines the Order struct representing an order placed in the
 * market. Includes timestamp, book side (bid/ask), price, quantity, 
 * associated order ID, and order status.
 * Author: Arvind Rathnashyam
 * Date: 2025-06-26
 * License: Proprietary
 */

#pragma once

#include "../types/book_side.h"
#include "../types/order_status.h"
#include "../types/order_type.h"
#include "../types/time_in_force.h"
#include "../types/usings.h"

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