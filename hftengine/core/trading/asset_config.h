/*
 * File: hftengine/hftengine/core/trading/asset_config.h
 * Description: Struct that holds all per-asset simulation parameters including 
 *              data paths, tick/lot sizes, latency model, fee structure, and fill behavior settings.
 * Author: Arvind Rathnashyam
 * Date: 2025-06-24
 * License: Proprietary
 */

# pragma once

# include <string>

struct AssetConfig {
    std::string l2update_file;
    std::string trade_file;

    double tick_size;
    double lot_size;
    double contract_multiplier;
    bool is_inverse;

    double maker_fee;
    double taker_fee;

    bool allow_partial_fills;
    double power_queue_model_exponent;
};