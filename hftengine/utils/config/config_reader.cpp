/*
 * File: config_reader.cpp
 * Description: File to create reader for config text files.
 * Author: Arvind Rathnashyam
 * Date: 2025-06-23
 * License: Proprietary
 */
#include <fstream>
#include <iostream>
#include <sstream>
#include <stdexcept>
#include <string>

#include "../../core/trading/asset_config.h"
#include "config_reader.h"

/*
 * opens a config reader for filename.txt formatted as :
 * key1=value1
 * key2=value2
 * ...
 * keys are strings, and values can be string, double, int
 */
ConfigReader::ConfigReader(const std::string &filename) {
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

AssetConfig ConfigReader::get_asset_config() const {
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