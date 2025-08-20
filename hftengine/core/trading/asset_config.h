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

namespace core {
namespace trading {
struct AssetConfig {
    std::string book_update_file_;
    std::string trade_file_;

    double tick_size_;
    double lot_size_;
    double contract_multiplier_;
    bool is_inverse_;

    double maker_fee_;
    double taker_fee_;
};
}
}