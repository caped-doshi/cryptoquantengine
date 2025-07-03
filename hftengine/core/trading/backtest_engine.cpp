/*
 * File: hftengine/core/execution_engine/backtest_engine.h
 * Description: Class structure for execution engine to take orders, fill
                    orders.
 * Author: Arvind Rathnashyam
 * Date: 2025-06-26
 * License: Proprietary
 */

#include <unordered_map>

#include "../data/readers/market_data_feed.h"
#include "asset_config.h"
#include "backtest_asset.h"
#include "backtest_engine.h"
#include "depth.h"

/**
 * @brief Constructs a BacktestEngine with per-asset book/trade streams and
 * configurations.
 *
 * This constructor initializes the market data feed and execution engine for
 * multiple assets using the provided CSV file paths and asset configuration
 * data. It also prepares internal state such as position tracking, trade
 * counters, and PnL for each asset.
 *
 * @param book_files A map from asset ID to CSV file path containing L2 book
 * updates.
 * @param trade_files A map from asset ID to CSV file path containing trade
 * events.
 * @param asset_configs A map from asset ID to AssetConfig objects defining tick
 * size, lot size, etc.
 *
 * @note All assets in @p asset_configs are expected to have corresponding
 * entries in @p book_files. Trade file entries are optional but recommended.
 */
BacktestEngine::BacktestEngine(
    const std::unordered_map<int, AssetConfig> &asset_configs)
    : current_time_us_(1), balance(0.0) {

    for (const auto &[asset_id, config] : asset_configs) {
        // Initialize BacktestAsset
        assets_.emplace(asset_id, BacktestAsset(config));

        // Initialize MarketDataFeed streams
        market_data_feed_.add_stream(asset_id, config.book_update_file_,
                                     config.trade_file_);

        // Initialize per-asset tracking state
        num_trades_[asset_id] = 0;
        trading_volume_[asset_id] = 0.0;
        trading_value_[asset_id] = 0.0;
        realized_pnl_[asset_id] = 0.0;
        position_[asset_id] = 0.0;
    }
}

/**
 * @brief Returns the current position for the specified asset.
 *
 * This function retrieves the net position of the given asset based on
 * previously executed trades or orders. A positive value indicates a net
 * long position, while a negative value indicates a net short position.
 *
 * @param asset_id The identifier of the asset.
 * @return The current position as a double.
 */
Quantity BacktestEngine::position(int asset_id) { return position_[asset_id]; }

Depth BacktestEngine::depth(int asset_id) { return Depth{0, 0, 0, 0}; }
