/*
 * File: config_reader.cpp
 * Description: File to create reader for config text files.
 * Author: Arvind Rathnashyam
 * Date: 2025-06-23
 * License: Proprietary
 */
#include "config_reader.hpp"
#include <string>
#include <iostream>
#include <fstream>
#include <sstream>
#include <stdexcept>

/*
 * opens a config reader for filename.txt formatted as : 
 * key1=value1
 * key2=value2
 * ...
 * keys are strings, and values can be string, double, int
 */
ConfigReader::ConfigReader(const std::string& filename) {
    std::ifstream file(filename);
    if (!file.is_open()) {
        throw std::runtime_error("Failed to open config file: " + filename);
    }
    std::string line;

    while (std::getline(file, line)) {
        if (line.empty() || line[0] == '#') continue; // Skip comments or blank lines

        std::istringstream iss(line);
        std::string key="", value = "";

        if (std::getline(iss, key, '=') && std::getline(iss, value)) {
            constants[key] = value;
        }
    }
}

/*
 * given a key, returns a string from filename if it exists.
 */
std::string ConfigReader::get_string(const std::string& key) const {
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
double ConfigReader::get_double(const std::string& key) const {
    return std::stod(get_string(key));
}

/*
 * given a key, returns a int from filename if exists.
 * calls get_string(key), and casts to int.
 */
int ConfigReader::get_int(const std::string& key) const {
    return std::stoi(get_string(key));
}

/*
 * given a key, checks if key exists in the config file
 */
bool ConfigReader::has(const std::string& key) const {
    return constants.find(key) != constants.end();
}