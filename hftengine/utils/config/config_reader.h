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

#include "../../core/strategy/gridtrading_config.h"
#include "../../core/trading/asset_config.h"

class ConfigReader {
  public:
    ConfigReader();

    AssetConfig get_asset_config(const std::string &filename);
    GridTradingConfig get_grid_trading_config(const std::string &filename);

  private:
    std::unordered_map<std::string, std::string> constants;

    std::string get_string(const std::string &key) const;
    double get_double(const std::string &key) const;
    int get_int(const std::string &key) const;
    bool has(const std::string &key) const;

    void load(const std::string &filename);
    void clear();
};