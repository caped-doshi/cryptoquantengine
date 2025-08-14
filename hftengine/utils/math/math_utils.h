/*
 * File: hftengine/utils/math/math_utils.h
 * Description: Math utility functions for HFT engine, including price/tick
 * conversions. 
 * Author: Arvind Rathnashyam
 * Date: 2025-08-13 
 * License: Proprietary
 */

#pragma once

#include "../../core/types/usings.h"
#include <cmath>

/**
 * @brief Converts a price to integer ticks given a tick size.
 *
 * This function rounds the price to the nearest tick and returns the tick
 * index.
 *
 * @param price The price to convert.
 * @param tick_size The tick size for the asset.
 * @return Ticks corresponding to the price.
 */
inline Ticks price_to_ticks(Price price, double tick_size) {
    return static_cast<Ticks>(std::round(price / tick_size));
}

/**
 * @brief Converts integer ticks to price given a tick size.
 *
 * @param ticks The tick index.
 * @param tick_size The tick size for the asset.
 * @return Price corresponding to the ticks.
 */
inline Price ticks_to_price(Ticks ticks, double tick_size) {
    return static_cast<Price>(ticks * tick_size);
}

/**
 * @brief Rounds a quantity to the nearest lot size.
 *
 * @param quantity The quantity to round.
 * @param lot_size The lot size for the asset.
 * @return Rounded quantity.
 */
inline Quantity quantity_to_lot(Quantity quantity, double lot_size) {
    constexpr double EPSILON = 1e-9;
    return std::round((quantity + EPSILON) / lot_size) * lot_size;
}