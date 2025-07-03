/*
 * File: tests/test_backtest_engine.cpp
 * Description: Unit tests for backtest_engine.cpp
 * Author: Arvind Rathnashyam
 * Date: 2025-07-02
 * License: Proprietary
 */

#include <catch2/catch_approx.hpp>
#include <catch2/catch_test_macros.hpp>
#include <filesystem>
#include <fstream>
#include <memory>

#include "core/execution_engine/execution_engine.h"
#include "core/market_data/book_update.h"
#include "core/orderbook/orderbook.h"
#include "core/trading/backtest_engine.h"
#include "core/types/book_side.h"
#include "core/types/order_type.h"
#include "core/types/time_in_force.h"
#include "core/types/update_type.h"
#include "core/types/usings.h"

namespace TestHelpers {
void create_trade_csv(const std::string &filename) {
    std::ofstream f(filename);
    f << "timestamp,local_timestamp,id,side,price,amount\n"
      << "100,110,1,buy,50000.0,1.0\n"
      << "300,310,2,sell,50001.0,0.5\n";
}

void create_book_update_csv(const std::string &filename) {
    std::ofstream f(filename);
    f << "timestamp,local_timestamp,is_snapshot,side,price,amount\n"
      << "200,210,false,bid,50000.0,2.0\n"
      << "400,410,false,ask,50001.0,1.5\n"
      << "500,510,false,ask,50001.0,2.5\n";
}
} // namespace TestHelpers

TEST_CASE("[BacktestEngine] - initializes correctly with valid input",
          "[backtest][init]") {
    const std::string book_file = "test_book.csv";
    const std::string trade_file = "test_trade.csv";
    TestHelpers::create_book_update_csv(book_file);
    TestHelpers::create_trade_csv(trade_file);

    // Config for a single asset
    int asset_id = 1;
    std::unordered_map<int, AssetConfig> asset_configs = {
        {asset_id, AssetConfig{.book_update_file_ = book_file,
                               .trade_file_ = trade_file,
                               .tick_size_ = 0.001,
                               .lot_size_ = 0.00001,
                               .contract_multiplier_ = 1.0,
                               .is_inverse_ = false,
                               .maker_fee_ = 0.0,
                               .taker_fee_ = 0.00045}}};

    SECTION("Construct BacktestEngine with valid file inputs") {
        bool threw = false;
        try {
            BacktestEngine engine(asset_configs);
        } catch (...) {
            threw = true;
        }
        REQUIRE(!threw);
    }

    // Cleanup
    std::filesystem::remove(book_file);
    std::filesystem::remove(trade_file);
}
