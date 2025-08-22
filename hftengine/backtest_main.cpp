/*
 * Copyright (c) 2025 Arvind Rathnashyam - arvindkrv@protonmail.com
 *
 * Please see the LICENSE file for the terms and conditions
 * associated with this software.
*/

#include <chrono>
#include <iomanip>
#include <iostream>
#include <memory>
#include <unordered_map>

#include "core/recorder/recorder.h"
#include "core/strategy/grid_trading/grid_trading.h"
#include "core/backtest_engine/backtest_engine.h"
#include "utils/config/config_reader.h"
#include "utils/logger/log_level.h"
#include "utils/logger/logger.h"

int main() {

    utils::config::ConfigReader config_reader;
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
    const std::unordered_map<int, core::trading::AssetConfig> asset_configs = {
        {asset_id, asset_config}};
    auto logger = nullptr;

    core::backtest::BacktestEngine engine(asset_configs, backtest_engine_config, logger);
    core::recorder::Recorder recorder(recorder_config.interval_us, logger);
    core::strategy::GridTrading grid_trading(asset_id, grid_trading_config, logger);

    const auto start = std::chrono::high_resolution_clock::now();

    std::uint64_t iter = backtest_config.iterations;
    while (engine.elapse(backtest_config.elapse_us) && iter-- > 0) {
        engine.clear_inactive_orders();
        grid_trading.on_elapse(engine);
        recorder.record(engine, asset_id);
    }

    const auto end = std::chrono::high_resolution_clock::now();
    const std::chrono::duration<double> elapsed = end - start;
    std::cout << "Backtest wall time: " << elapsed.count() << " seconds\n";

    std::cout << "Final equity: " << std::fixed << std::setprecision(2)
              << engine.equity() << "\n";
    recorder.print_performance_metrics();
    engine.print_trading_stats(asset_id);

    recorder.plot(asset_id);

    return 0;
}
