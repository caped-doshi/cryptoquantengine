/*
 * File: hftengine/backtest_main.cpp
 * Description: Loads configurations and runs the backtest engine.
 * Author: Arvind Rathnashyam
 * Date: 2025-08-15
 * License: Proprietary
 */

#include <memory>
#include <iostream>
#include <unordered_map>

#include "utils/logger/logger.h"
#include "core/trading/backtest_engine.h"
#include "core/recorder/recorder.h"
#include "core/strategy/grid_trading.h"
#include "utils/config/config_reader.h"

int main() {

    ConfigReader config_reader;
    auto asset_config = config_reader.get_asset_config("asset_config.txt");
    auto grid_trading_config =
        config_reader.get_grid_trading_config("grid_trading_config.txt");
    std::unordered_map<int, AssetConfig> asset_configs;
    asset_configs.insert({1, asset_config});
    auto logger = std::make_shared<Logger>("backtest.log");

    BacktestEngine backtest_engine(asset_configs, logger);
    Recorder recorder(1'000'000, logger); // 1 second intervals



    return 0;
}
