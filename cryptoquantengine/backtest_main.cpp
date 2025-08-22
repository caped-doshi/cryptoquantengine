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
#include <string>
#include <unordered_map>

#include "core/backtest_engine/backtest_engine.h"
#include "core/recorder/recorder.h"
#include "core/strategy/grid_trading/grid_trading.h"
#include "utils/config/config_reader.h"
#include "utils/logger/log_level.h"
#include "utils/logger/logger.h"

int main(int argc, char *argv[]) {
    try {
        // Configurable paths (default or command line)
        std::string asset_cfg =
            (argc > 1) ? argv[1] : "../config/asset_config.txt";
        std::string grid_cfg =
            (argc > 2) ? argv[2] : "../config/grid_trading_config.txt";
        std::string bt_engine_cfg =
            (argc > 3) ? argv[3] : "../config/backtest_engine_config.txt";
        std::string recorder_cfg =
            (argc > 4) ? argv[4] : "../config/recorder_config.txt";
        std::string bt_cfg =
            (argc > 5) ? argv[5] : "../config/backtest_config.txt";

        // Initialize logger
        auto logger = std::make_shared<utils::logger::Logger>(
            "backtest.log", utils::logger::LogLevel::Info);

        // Read configs
        utils::config::ConfigReader config_reader;
        const auto asset_config = config_reader.get_asset_config(asset_cfg);
        const auto grid_trading_config =
            config_reader.get_grid_trading_config(grid_cfg);
        const auto backtest_engine_config =
            config_reader.get_backtest_engine_config(bt_engine_cfg);
        const auto recorder_config =
            config_reader.get_recorder_config(recorder_cfg);
        const auto backtest_config = config_reader.get_backtest_config(bt_cfg);

        // Asset setup (single asset example)
        const int asset_id = 1;
        const std::unordered_map<int, core::trading::AssetConfig>
            asset_configs = {{asset_id, asset_config}};

        // Engine, recorder, and strategy initialization
        core::backtest::BacktestEngine engine(asset_configs,
                                              backtest_engine_config, logger);
        core::recorder::Recorder recorder(recorder_config.interval_us, logger);
        core::strategy::GridTrading grid_trading(asset_id, grid_trading_config,
                                                 logger);

        // Backtest loop
        const auto start = std::chrono::high_resolution_clock::now();
        std::uint64_t iter = backtest_config.iterations;
        while (engine.elapse(backtest_config.elapse_us) && iter-- > 0) {
            engine.clear_inactive_orders();
            grid_trading.on_elapse(engine);
            recorder.record(engine, asset_id);
        }
        const auto end = std::chrono::high_resolution_clock::now();
        const std::chrono::duration<double> elapsed = end - start;

        // Results
        std::cout << "Backtest wall time: " << elapsed.count() << " seconds\n";
        std::cout << "Final equity: " << std::fixed << std::setprecision(2)
                  << engine.equity() << "\n";
        recorder.print_performance_metrics();
        engine.print_trading_stats(asset_id);
        recorder.plot(asset_config.name_);

        return 0;
    } catch (const std::exception &e) {
        std::cerr << "Exception: " << e.what() << std::endl;
        return 1;
    }
}