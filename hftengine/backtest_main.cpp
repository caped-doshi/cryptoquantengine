/*
 * File: hftengine/backtest_main.cpp
 * Description: Loads configurations and runs the backtest engine.
 * Author: Arvind Rathnashyam
 * Date: 2025-08-15
 * License: Proprietary
 */

#include <memory>
#include <iostream>

#include "utils/logger/logger.h"
#include "core/trading/backtest_engine.h"
#include "core/recorder/recorder.h"
#include "core/strategy/grid_trading.h"
#include "utils/config/config_reader.h"

int main() {

    auto logger = std::make_shared<Logger>("backtest.log");

    return 0;
}
