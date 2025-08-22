/*
 * Copyright (c) 2025 arvindkrv@protonmail.com
 *
 * Please see the LICENSE file for the terms and conditions
 * associated with this software.
 */

#pragma once

#include <string>

namespace core::trading {
struct AssetConfig {
    std::string book_update_file_;
    std::string trade_file_;

    double tick_size_;
    double lot_size_;
    double contract_multiplier_;
    bool is_inverse_;

    double maker_fee_;
    double taker_fee_;

    std::string name_;
};
} // namespace core::trading