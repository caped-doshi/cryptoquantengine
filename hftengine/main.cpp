/*
 * File: main.cpp
 * Usage: build/mingw32-make
 * Description: Loads data and calls backtester.
 * Author: Arvind Rathnashyam
 * Date: 2025-06-23
 * License: Proprietary
 */

# include <iostream>
# include <iomanip>

# include "utils/config/config_reader.hpp"

int main() {
	std::cout << "[MAIN] -- opened config.txt" << std::endl;
	ConfigReader config("config.txt");
	const double initial_cash = config.get_double("initial_cash");
	const double future_taker_fee = config.get_double("future_taker_fee");
	const double future_maker_fee = config.get_double("future_maker_fee");

	std::cout << "===== CONFIGURATION =====" << std::endl;
	std::cout << "Execution Engine: " << std::endl;
	std::cout << "  future taker fee : " << std::fixed << std::setprecision(5) << future_taker_fee << std::endl;
	std::cout << "  future maker fee : " << std::fixed << std::setprecision(5) << future_maker_fee << std::endl;
	std::cout << "=========================" << std::endl;

}

