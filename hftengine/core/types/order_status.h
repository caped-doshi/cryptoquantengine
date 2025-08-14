/*
 * File: hftengine/core/types/order_status.h
 * Description: Enum class defining the order status types.
 * Author: Arvind Rathnashyam
 * Date: 2025-08-11
 * License: Proprietary
 */

#pragma once

enum class OrderStatus {
    NEW, 
    ACTIVE,
    PARTIALLY_FILLED,
    FILLED,
    CANCELLED,
    REJECTED,
    EXPIRED
};