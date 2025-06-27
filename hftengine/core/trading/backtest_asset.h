/*
 * File: hftengine/hftengine/core/trading/backtest_asset.h
 * Description: BacktestAsset encapsulates all per-instrument simulation parameters, data sources, and execution                 models used in the backtest engine.
 * Author: Arvind Rathnashyam
 * Date: 2025-06-26
 * License: Proprietary
 */

# include <cstdint>
# include <vector>

# include "asset_config.h"
# include "../types/usings.h"
# include "../types/trade_side.h"

class BacktestAsset {
public:
    explicit BacktestAsset(const AssetConfig& cfg)
        : cfg_(cfg) {}

    const AssetConfig& config() const {
        return cfg_;
    }

private:
    AssetConfig cfg_;
};
