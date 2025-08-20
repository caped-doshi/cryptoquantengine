/*
 * File: tests/strategies/test_grid_trading.cpp
 * Description: Unit tests for GridTrading strategy.
 * Author: Arvind Rathnashyam
 * Date: 2025-08-12
 * License: Proprietary
 */

#include <catch2/catch_approx.hpp>
#include <catch2/catch_test_macros.hpp>
#include <filesystem>
#include <fstream>
#include <memory>

#include "core/execution_engine/execution_engine.h"
#include "core/strategy/grid_trading.h"
#include "core/backtest_engine/backtest_engine.h"
#include "utils/logger/log_level.h"
#include "utils/logger/logger.h"
#include "utils/math/math_utils.h"

namespace TestHelpers {
void create_trade_csv(const std::string &filename) {
    std::ofstream f(filename);
    f << "timestamp,local_timestamp,id,side,price,amount\n"
      << "10000,11000,1,buy,50000.0,1.0\n"
      << "11000,12000,2,buy,50000.5,1.0\n"
      << "45000,46000,5,buy,50001.0,5.0\n"
      << "45500,46500,7,buy,50001.1,5.0\n"
      << "46000,47000,8,buy,50001.2,5.0\n";
}

void create_book_update_csv(const std::string &filename) {
    std::ofstream f(filename);
    f << "timestamp,local_timestamp,is_snapshot,side,price,amount\n"
      << "1000,2000,false,ask,50001.0,1.5\n"
      << "20000,21000,false,bid,50000.0,2.0\n"
      << "30000,31000,false,bid,50000.5,2.0\n"
      << "40000,41000,false,ask,50001.0,2.5\n"
      << "42500,43500,false,ask,50001.0,0.0\n";
}
} // namespace TestHelpers

TEST_CASE("[GridTrading] - does not submit orders when notional too small",
          "[grid-trading][notional]") {
    using namespace core::trading;
    using namespace core::backtest;
    using namespace core::strategy;
    // Setup as before, but with very small notional
    const double notional_order_qty = 0.00001;

    const std::string book_file = "test_grid_book.csv";
    const std::string trade_file = "test_grid_trade.csv";
    TestHelpers::create_book_update_csv(book_file);
    TestHelpers::create_trade_csv(trade_file);

    const int asset_id = 1;
    const double tick_size = 0.01;
    const double lot_size = 0.00001;
    const int grid_num = 3;
    const Ticks grid_interval = 10;
    const Ticks half_spread = 20;
    const double position_limit = 10.0;
    const Microseconds order_entry_latency_us = 1000;
    const Microseconds order_response_latency_us = 1000;

    std::unordered_map<int, AssetConfig> asset_configs = {
        {asset_id, AssetConfig{.book_update_file_ = book_file,
                               .trade_file_ = trade_file,
                               .tick_size_ = tick_size,
                               .lot_size_ = lot_size,
                               .contract_multiplier_ = 1.0,
                               .is_inverse_ = false,
                               .maker_fee_ = 0.0,
                               .taker_fee_ = 0.0}}};
    auto backtest_engine_config = BacktestEngineConfig{
        .initial_cash_ = 1000.0,
        .order_entry_latency_us_ = order_entry_latency_us,
        .order_response_latency_us_ = order_response_latency_us,
        .market_feed_latency_us_ = 1000};
    auto logger = std::make_shared<utils::logger::Logger>("test_grid_trading_notional.log",utils::logger::LogLevel::Debug);
    BacktestEngine hbt(asset_configs, backtest_engine_config, logger);

    GridTrading strat(asset_id, grid_num, grid_interval, half_spread,
                      position_limit, notional_order_qty, logger);

    REQUIRE(hbt.elapse(42000));
    strat.initialize();

    strat.on_elapse(hbt);
    REQUIRE(hbt.elapse(2100));

    auto orders = hbt.orders(asset_id);
    REQUIRE(orders.empty());

    std::filesystem::remove(book_file);
    std::filesystem::remove(trade_file);
}

TEST_CASE("[GridTrading] - cancels orders not in new grid",
          "[grid-trading][cancel]") {
    using namespace core::trading;
    using namespace core::backtest;
    using namespace core::strategy;

    const std::string book_file = "test_grid_book.csv";
    const std::string trade_file = "test_grid_trade.csv";
    TestHelpers::create_book_update_csv(book_file);
    TestHelpers::create_trade_csv(trade_file);

    const int asset_id = 1;
    const double tick_size = 0.1;
    const double lot_size = 0.00001;
    const int grid_num = 3;
    const Ticks grid_interval = 10;
    const Ticks half_spread = 20;
    const double position_limit = 10.0;
    const double notional_order_qty = 100.0;
    const Microseconds order_entry_latency_us = 1000;
    const Microseconds order_response_latency_us = 1000;

    std::unordered_map<int, AssetConfig> asset_configs = {
        {asset_id, AssetConfig{.book_update_file_ = book_file,
                               .trade_file_ = trade_file,
                               .tick_size_ = tick_size,
                               .lot_size_ = lot_size,
                               .contract_multiplier_ = 1.0,
                               .is_inverse_ = false,
                               .maker_fee_ = 0.0,
                               .taker_fee_ = 0.0}}};
    auto backtest_engine_config = BacktestEngineConfig{
        .initial_cash_ = 1000.0,
        .order_entry_latency_us_ = order_entry_latency_us,
        .order_response_latency_us_ = order_response_latency_us,
        .market_feed_latency_us_ = 1000};
    auto logger = std::make_shared<utils::logger::Logger>("test_grid_trading_cancel.log",
                                           utils::logger::LogLevel::Debug);
    BacktestEngine hbt(asset_configs, backtest_engine_config, logger);
    hbt.set_order_entry_latency(order_entry_latency_us);
    hbt.set_order_response_latency(order_response_latency_us);

    GridTrading strat(asset_id, grid_num, grid_interval, half_spread,
                      position_limit, notional_order_qty, logger);

    REQUIRE(hbt.elapse(42000));
    strat.initialize();

    // Submit an order outside the grid
    hbt.submit_buy_order(asset_id, 49900.0, lot_size, TimeInForce::GTC,
                         OrderType::LIMIT);

    strat.on_elapse(hbt);

    // After on_elapse, the order should be cancelled
    auto orders = hbt.orders(asset_id);
    for (const auto &order : orders) {
        REQUIRE(order.price_ != 49900.0);
    }

    std::filesystem::remove(book_file);
    std::filesystem::remove(trade_file);
}

TEST_CASE("[GridTrading] - on_elapse submits correct grid orders",
          "[grid-trading][elapse]") {
    using namespace core::trading;
    using namespace core::backtest;
    using namespace core::strategy;

    const std::string book_file = "test_grid_book.csv";
    const std::string trade_file = "test_grid_trade.csv";
    TestHelpers::create_book_update_csv(book_file);
    TestHelpers::create_trade_csv(trade_file);

    const int asset_id = 1;
    const double tick_size = 0.01;
    const double lot_size = 0.00001;
    const int grid_num = 3;
    const Ticks grid_interval = 10;
    const Ticks half_spread = 20;
    const double position_limit = 10.0;
    const double notional_order_qty = 100.0;
    const Microseconds order_entry_latency_us = 1000;
    const Microseconds order_response_latency_us = 1000;

    std::unordered_map<int, AssetConfig> asset_configs = {
        {asset_id, AssetConfig{.book_update_file_ = book_file,
                               .trade_file_ = trade_file,
                               .tick_size_ = tick_size,
                               .lot_size_ = lot_size,
                               .contract_multiplier_ = 1.0,
                               .is_inverse_ = false,
                               .maker_fee_ = 0.0,
                               .taker_fee_ = 0.0}}};
    auto backtest_engine_config = BacktestEngineConfig{
        .initial_cash_ = 1000.0,
        .order_entry_latency_us_ = order_entry_latency_us,
        .order_response_latency_us_ = order_response_latency_us,
        .market_feed_latency_us_ = 1000};
    auto logger = std::make_shared<utils::logger::Logger>("test_grid_trading_elapse.log",
                                           utils::logger::LogLevel::Debug);

    BacktestEngine hbt(asset_configs, backtest_engine_config, logger);
    hbt.set_order_entry_latency(order_entry_latency_us);
    hbt.set_order_response_latency(order_response_latency_us);

    GridTrading strat(asset_id, grid_num, grid_interval, half_spread,
                      position_limit, notional_order_qty, logger);

    // Advance engine to load book and trades
    REQUIRE(hbt.elapse(42000));
    strat.initialize();

    // Call on_elapse to trigger grid order submission
    strat.on_elapse(hbt);

    REQUIRE(hbt.elapse(2100));

    // Check that grid orders are submitted
    auto orders = hbt.orders(asset_id);
    REQUIRE(orders.size() == 6); // 3 bids + 3 asks

    Quantity expected_qty =
        std::round((notional_order_qty / 50000.75) / lot_size) * lot_size;

    // Check order properties
    for (const auto &order : orders) {
        REQUIRE(order.orderType_ == OrderType::LIMIT);
        REQUIRE(order.tif_ == TimeInForce::GTC);
        REQUIRE(order.quantity_ == Catch::Approx(expected_qty).margin(1e-8));
        REQUIRE((order.side_ == BookSide::Bid || order.side_ == BookSide::Ask));
    }

    // Cleanup
    std::filesystem::remove(book_file);
    std::filesystem::remove(trade_file);
}

TEST_CASE("[GridTrading] - handles position limits correctly",
          "[grid-trading][position-limit]") {
    using namespace core::trading;
    using namespace core::backtest;
    using namespace core::strategy;

    const std::string book_file = "test_grid_book.csv";
    const std::string trade_file = "test_grid_trade.csv";
    TestHelpers::create_book_update_csv(book_file);
    TestHelpers::create_trade_csv(trade_file);
    const int asset_id = 1;
    const double tick_size = 0.01;
    const double lot_size = 0.01;
    const int grid_num = 3;
    const Ticks grid_interval = 10;
    const Ticks half_spread = 20;
    const double position_limit = 5.0; // Limit of 10 units
    const double notional_order_qty = 100000.0;
    const Microseconds order_entry_latency_us = 1000;
    const Microseconds order_response_latency_us = 1000;
    std::unordered_map<int, AssetConfig> asset_configs = {
        {asset_id, AssetConfig{.book_update_file_ = book_file,
                               .trade_file_ = trade_file,
                               .tick_size_ = tick_size,
                               .lot_size_ = lot_size,
                               .contract_multiplier_ = 1.0,
                               .is_inverse_ = false,
                               .maker_fee_ = 0.0,
                               .taker_fee_ = 0.0}}};
    auto backtest_engine_config = BacktestEngineConfig{
        .initial_cash_ = 1000.0,
        .order_entry_latency_us_ = order_entry_latency_us,
        .order_response_latency_us_ = order_response_latency_us,
        .market_feed_latency_us_ = 1000};
    auto logger = std::make_shared<utils::logger::Logger>(
        "test_grid_trading_position_limit.log", utils::logger::LogLevel::Debug);

    BacktestEngine hbt(asset_configs, backtest_engine_config, logger);
    GridTrading strat(asset_id, grid_num, grid_interval, half_spread,
                      position_limit, notional_order_qty, logger);
    // Advance engine to load book and trades
    REQUIRE(hbt.elapse(42000));
    strat.initialize();
    strat.on_elapse(hbt);
    // Advance simulated time and verify orders placed
    REQUIRE(hbt.elapse(2100));
    REQUIRE(hbt.current_time() == 44100);
    REQUIRE(hbt.orders(asset_id).size() == 6); // 3 bids + 3 asks
    // Advance simulated time and verify ask orders are filled
    REQUIRE(hbt.elapse(5900));
    REQUIRE(hbt.current_time() == 50000);
    REQUIRE(hbt.position(asset_id) == Catch::Approx(-6.0).margin(1e-8));
    hbt.clear_inactive_orders();
    // Verify that no new ask orders are placed due to position limit
    strat.on_elapse(hbt);
    REQUIRE(hbt.elapse(2100));
    REQUIRE(hbt.orders(asset_id).size() == 3);
    for (const auto &order : hbt.orders(asset_id)) {
        REQUIRE(order.side_ == BookSide::Bid);
    }
    // Cleanup
    std::filesystem::remove(book_file);
    std::filesystem::remove(trade_file);
}