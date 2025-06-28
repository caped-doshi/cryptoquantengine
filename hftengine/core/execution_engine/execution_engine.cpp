/*
 * File: hftengine/core/execution_engine/execution_engine.cpp
 * Description: Class for execution engine to take orders, fill
 * orders.
 * Author: Arvind Rathnashyam
 * Date: 2025-06-26
 * License: Proprietary
 */

#include <unordered_map>
#include <vector>

#include "../types/book_side.h"
#include "../types/order_type.h"
#include "../types/usings.h"
#include "execution_engine.h"

ExecutionEngine::ExecutionEngine() {}

bool submit_buy_order(int asset_id, const Timestamp &timestamp,
                      const OrderId &orderId, const Price &price,
                      const Quantity &quantity, const TimeInForce &tif,
                      const OrderType &orderType) {
    if (quantity <= 0.0 || price <= 0.0) return false;
    Order order = {timestamp, orderId, BookSide::Bid, price,
                   quantity,  tif,     orderType};
    bid_orders_[orderId] = order;
    return true;
}

bool submit_sell_order(int asset_id, const Timestamp &timestamp,
                       const OrderId &orderId, const Price &price,
                       const Quantity &quantity, const TimeInForce &tif,
                       const OrderType &orderType) {
    if (quantity <= 0.0 || price <= 0.0) return false;
    Order order = {timestamp, orderId, BookSide::Ask, price,
                   quantity,  tif,     orderType};
    ask_orders_[orderId] = order;
    return true;
}