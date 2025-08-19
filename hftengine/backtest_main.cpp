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
#include "core/trading/backtest_engine.h"
#include "utils/config/config_reader.h"
#include "utils/logger/log_level.h"
#include "utils/logger/logger.h"

int main() {

    ConfigReader config_reader;
    auto asset_config =
        config_reader.get_asset_config("../config/asset_config.txt");
    auto grid_trading_config = config_reader.get_grid_trading_config(
        "../config/grid_trading_config.txt");
    auto backtest_engine_config = config_reader.get_backtest_engine_config(
        "../config/backtest_engine_config.txt");
    std::unordered_map<int, AssetConfig> asset_configs;
    const int asset_id = 1;
    asset_configs.insert({asset_id, asset_config});
    // auto logger = std::make_shared<Logger>("backtest.log", LogLevel::Warning);
    auto logger = nullptr;

    double initial_cash = 1'000.0;
    BacktestEngine hbt(asset_configs, backtest_engine_config, logger);
    Recorder recorder(100'000, logger);
    GridTrading grid_trading(1, grid_trading_config, logger);

    auto start = std::chrono::high_resolution_clock::now();

    std::uint64_t iter = 86400;
    while (hbt.elapse(100'000) && iter-- > 0) {
        hbt.clear_inactive_orders();
        grid_trading.on_elapse(hbt);
        recorder.record(hbt, asset_id);
    }

    auto end = std::chrono::high_resolution_clock::now();
    std::chrono::duration<double> elapsed = end - start;
    std::cout << "Backtest wall time: " << elapsed.count() << " seconds\n";

    std::cout << "Final equity: " << std::fixed << std::setprecision(2)
              << hbt.equity() << "\n";
    recorder.print_performance_metrics();
    hbt.print_trading_stats(asset_id);

    recorder.plot(asset_id);

    return 0;
}
