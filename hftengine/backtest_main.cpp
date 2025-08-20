/*
 * File: hftengine/backtest_main.cpp
 * Description: Loads configurations and runs the backtest engine.
 * Author: Arvind Rathnashyam
 * Date: 2025-08-15
 * License: Proprietary
 */

#include <chrono>
#include <iostream>
#include <memory>
#include <unordered_map>

#include "core/recorder/recorder.h"
#include "core/strategy/grid_trading.h"
#include "core/backtest_engine/backtest_engine.h"
#include "utils/config/config_reader.h"
#include "utils/logger/log_level.h"
#include "utils/logger/logger.h"

int main() {
    using namespace core::trading;
    using namespace core::backtest;
    using namespace core::recorder;
    using namespace core::strategy;
    using namespace utils::config;

    ConfigReader config_reader;
    const auto asset_config =
        config_reader.get_asset_config("../config/asset_config.txt");
    const auto grid_trading_config = config_reader.get_grid_trading_config(
        "../config/grid_trading_config.txt");
    const auto backtest_engine_config = config_reader.get_backtest_engine_config(
        "../config/backtest_engine_config.txt");
    const auto recorder_config =
        config_reader.get_recorder_config("../config/recorder_config.txt");
    const auto backtest_config =
        config_reader.get_backtest_config("../config/backtest_config.txt");
    const int asset_id = 1;
    const std::unordered_map<int, AssetConfig> asset_configs = {
        {asset_id, asset_config}};
    auto logger = nullptr;

    BacktestEngine hbt(asset_configs, backtest_engine_config, logger);
    Recorder recorder(recorder_config.interval_us, logger);
    GridTrading grid_trading(asset_id, grid_trading_config, logger);

    const auto start = std::chrono::high_resolution_clock::now();

    std::uint64_t iter = backtest_config.iterations;
    while (hbt.elapse(backtest_config.elapse_us) && iter-- > 0) {
        hbt.clear_inactive_orders();
        grid_trading.on_elapse(hbt);
        recorder.record(hbt, asset_id);
    }

    const auto end = std::chrono::high_resolution_clock::now();
    const std::chrono::duration<double> elapsed = end - start;
    std::cout << "Backtest wall time: " << elapsed.count() << " seconds\n";

    std::cout << "Final equity: " << std::fixed << std::setprecision(2)
              << hbt.equity() << "\n";
    recorder.print_performance_metrics();
    hbt.print_trading_stats(asset_id);

    recorder.plot(asset_id);

    return 0;
}
