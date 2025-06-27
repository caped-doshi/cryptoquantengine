/*
 * File: hft_bt_engine/core/market_data/core/time_in_force.h
 * Description: Enum class defining the different time in force types, GTC, GTX, FOK, IOC.
 * Author: Arvind Rathnashyam
 * Date: 2025-06-27
 * License: Proprietary
 */

#pragma once

enum class TimeInForce
{
    GTC, // good till cancel
    GTX, // good till crossing
    FOK, // fill or cancel
    IOC  // immediate or cancel
};