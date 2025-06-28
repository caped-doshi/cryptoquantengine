/*
 * File: hft_bt_engine/core/trading/fill.h
 * Description: Fill represents the execution details of a matched order.
 * associated order ID.
 * Author: Arvind Rathnashyam
 * Date: 2025-06-26
 * License: Proprietary
 */

#pragma once

#include "../types/book_side.h"
#include "../types/trade_side.h"
#include "../types/order_type.h"
#include "../types/time_in_force.h"
#include "../types/usings.h"

struct Fill {
    int asset_id_;
    Timestamp timestamp_;
    OrderId orderId_;
    TradeSide side_;
    Price price_;
    Quantity quantity_;

    double fee;
    bool is_maker;
};