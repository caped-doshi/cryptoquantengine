/*
 * File: config_reader.cpp
 * Description: File to create reader for config text files.
 * Author: Arvind Rathnashyam
 * Date: 2025-06-23
 * License: Proprietary
 */
#include <cstdint>
#include <fstream>
#include <iostream>
#include <sstream>
#include <stdexcept>
#include <string>

#include "../../core/recorder/recorder_config.h"
#include "../../core/strategy/grid_trading_config.h"
#include "../../core/trading/asset_config.h"
#include "../../core/trading/backtest_config.h"
#include "../../core/trading/backtest_engine_config.h"
#include "config_reader.h"

namespace utils {
namespace config {

ConfigReader::ConfigReader() {}

/*
 * opens a config reader for filename.txt formatted as :
 * key1=value1
 * key2=value2
 * ...
 * keys are strings, and values can be string, double, int
 */
void ConfigReader::load(const std::string &filename) {
    std::ifstream file(filename);
    if (!file.is_open()) {
        throw std::runtime_error("Failed to open config file: " + filename);
    }
    std::string line;

    while (std::getline(file, line)) {
        if (line.empty() || line[0] == '#')
            continue; // Skip comments or blank lines

        std::istringstream iss(line);
        std::string key = "", value = "";

        if (std::getline(iss, key, '=') && std::getline(iss, value)) {
            constants[key] = value;
        }
    }
}

/*
 * @brief clears the config reader, removing all constants.
 */
void ConfigReader::clear() { constants.clear(); }

/*
 * given a key, returns a string from filename if it exists.
 */
std::string ConfigReader::get_string(const std::string &key) const {
    auto it = constants.find(key);
    if (it != constants.end()) {
        return it->second;
    }
    throw std::runtime_error("Key not found: " + key);
}

/*
 * given a key, returns a double from filename if exists.
 * calls get_string(key), and casts to double.
 */
double ConfigReader::get_double(const std::string &key) const {
    std::string value = get_string(key);
    try {
        return std::stod(value);
    } catch (const std::exception &ex) {
        throw std::invalid_argument("Failed to convert key '" + key +
                                    "' with value '" + value +
                                    "' to double: " + ex.what());
    }
}

/*
 * given a key, returns a int from filename if exists.
 * calls get_string(key), and casts to int.
 */
int ConfigReader::get_int(const std::string &key) const {
    std::string value = get_string(key);
    try {
        return std::stoi(value);
    } catch (const std::exception &ex) {
        throw std::invalid_argument("Failed to convert key '" + key +
                                    "' with value '" + value +
                                    "' to int: " + ex.what());
    }
}

/*
 * given a key, checks if key exists in the config file
 */
bool ConfigReader::has(const std::string &key) const {
    return constants.find(key) != constants.end();
}
/*
 * @brief Reads the asset configuration from a file.
 * @param filename The name of the configuration file.
 * @return An AssetConfig object containing the asset configuration.*
 */
AssetConfig ConfigReader::get_asset_config(const std::string &filename) {
    clear();
    load(filename);
    AssetConfig config;
    config.book_update_file_ = get_string("book_update_file");
    config.trade_file_ = get_string("trade_file");
    config.tick_size_ = get_double("tick_size");
    config.lot_size_ = get_double("lot_size");
    config.contract_multiplier_ = get_double("contract_multiplier");
    config.is_inverse_ = get_int("is_inverse") != 0;
    config.maker_fee_ = get_double("maker_fee");
    config.taker_fee_ = get_double("taker_fee");
    return config;
}
/*
 * @brief Reads the grid trading configuration from a file.
 */
GridTradingConfig
ConfigReader::get_grid_trading_config(const std::string &filename) {
    clear();
    load(filename);
    // read the grid trading config from the file
    GridTradingConfig config;
    config.tick_size_ = get_double("tick_size");
    config.lot_size_ = get_double("lot_size");
    config.grid_num_ = get_int("grid_num");
    config.grid_interval_ = static_cast<Ticks>(get_int("grid_interval"));
    config.half_spread_ = static_cast<Ticks>(get_int("half_spread"));
    config.position_limit_ = get_double("position_limit");
    config.notional_order_qty_ = get_double("notional_order_qty");
    return config;
}
/*
 * @brief Reads the backtest engine configuration from a file.
 */
BacktestEngineConfig
ConfigReader::get_backtest_engine_config(const std::string &filename) {
    clear();
    load(filename);
    // read the backtest engine config from the file
    BacktestEngineConfig config;
    config.initial_cash_ = get_double("initial_cash");
    config.order_entry_latency_us_ = get_int("order_entry_latency_us");
    config.order_response_latency_us_ = get_int("order_response_latency_us");
    config.market_feed_latency_us_ = get_int("market_feed_latency_us");
    return config;
}
/*
 * @brief Reads the recorder configuration from a file.
 */
RecorderConfig ConfigReader::get_recorder_config(const std::string &filename) {
    clear();
    load(filename);
    RecorderConfig config;
    config.interval_us = has("interval_us") ? get_int("interval_us") : 1000000;
    config.output_file =
        has("output_file") ? get_string("output_file") : "recorder_output.csv";
    return config;
}
/*
 * @brief Reads the backtest configuration from a file.
 */
BacktestConfig ConfigReader::get_backtest_config(const std::string &filename) {
    clear();
    load(filename);
    BacktestConfig config;
    config.elapse_us = has("elapse_us") ? get_int("elapse_us") : 1000000;
    config.iterations = has("iterations") ? get_int("iterations") : 86400;
    return config;
}

} // namespace config
} // namespace utils