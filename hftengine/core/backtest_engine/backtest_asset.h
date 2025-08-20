/*
 * File: hftengine/hftengine/core/backtest_engine/backtest_asset.h
 * Description: BacktestAsset encapsulates all per-instrument simulation
 * parameters, data sources, and execution models used in the
 * backtest engine.
 * Author: Arvind Rathnashyam
 * Date: 2025-06-26
 * License: Proprietary
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