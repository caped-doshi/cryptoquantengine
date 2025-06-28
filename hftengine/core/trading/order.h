/*
 * File: hft_bt_engine/core/trading/order.h
 * Description: Defines the Order struct representing an order placed in the
 * market. Includes timestamp, book side (bid/ask), price, quantity, and
 * associated order ID. 
 * Author: Arvind Rathnashyam 
 * Date: 2025-06-26 
 * License: Proprietary
 */

#pragma once

#include "../types/book_side.h"
#include "../types/order_type.h"
#include "../types/time_in_force.h"
#include "../types/usings.h"

struct Order {
    Timestamp timestamp_;
    OrderId orderId_;
    BookSide side_;
    Price price_;
    Quantity quantity_;
    TimeInForce tif_;
    OrderType orderType_;
};