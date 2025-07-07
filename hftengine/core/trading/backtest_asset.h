/*
 * File: hftengine/hftengine/core/trading/backtest_asset.h
 * Description: BacktestAsset encapsulates all per-instrument simulation
 * parameters, data sources, and execution                 models used in the
 * backtest engine. Author: Arvind Rathnashyam Date: 2025-06-26 License:
 * Date: 2025-06-26
 * License: Proprietary
 */

#pragma once

#include <cstdint>
#include <vector>

#include "../types/trade_side.h"
#include "../types/usings.h"
#include "asset_config.h"

class BacktestAsset {
  public:
    BacktestAsset() = default;
    explicit BacktestAsset(const AssetConfig &config) : config_(config) {}

    const AssetConfig &config() const { return config_; }

  private:
    AssetConfig config_;
};
