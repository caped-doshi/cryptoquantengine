/*
 * File: config_reader.hpp
 * Description: Contains header functions for config_reader.cpp.
 * Author: Arvind Rathnashyam
 * Date: 2025-06-23
 * License: Proprietary
 */
#pragma once

#include <string>
#include <unordered_map>

#include "../../core/recorder/recorder_config.h"
#include "../../core/strategy/grid_trading_config.h"
#include "../../core/trading/asset_config.h"
#include "../../core/backtest_engine/backtest_config.h"
#include "../../core/backtest_engine/backtest_engine_config.h"

namespace utils::config {

class ConfigReader {
  public:
    ConfigReader();

    core::trading::AssetConfig get_asset_config(const std::string &filename);
    core::strategy::GridTradingConfig get_grid_trading_config(const std::string &filename);
    core::backtest::BacktestEngineConfig
    get_backtest_engine_config(const std::string &filename);
    RecorderConfig get_recorder_config(const std::string &filename);
    core::backtest::BacktestConfig get_backtest_config(const std::string &filename);

  private:
    std::unordered_map<std::string, std::string> constants;

    std::string get_string(const std::string &key) const;
    double get_double(const std::string &key) const;
    int get_int(const std::string &key) const;
    bool has(const std::string &key) const;

    void load(const std::string &filename);
    void clear();
};
}