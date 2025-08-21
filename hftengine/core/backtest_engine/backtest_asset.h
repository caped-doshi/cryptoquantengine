/*
 * Copyright (c) 2025 arvindkrv@protonmail.com
 *
 * Please see the LICENSE file for the terms and conditions
 * associated with this software.
 */

#pragma once

#include <cstdint>
#include <vector>

#include "../trading/asset_config.h"

namespace core::backtest {
class BacktestAsset {
  public:
    BacktestAsset() = default;
    explicit BacktestAsset(const core::trading::AssetConfig &config)
        : config_(config) {}

    const core::trading::AssetConfig &config() const { return config_; }

  private:
    core::trading::AssetConfig config_;
};
} // namespace core::backtest