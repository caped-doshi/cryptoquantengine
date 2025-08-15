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

#include "../../core/trading/asset_config.h"

class ConfigReader {
private:
    std::unordered_map<std::string, std::string> constants;

public:
    ConfigReader(const std::string& filename);

    std::string get_string(const std::string& key) const;
    double get_double(const std::string& key) const;
    int get_int(const std::string& key) const;

    bool has(const std::string& key) const;

    AssetConfig get_asset_config() const;
};