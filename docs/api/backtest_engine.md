# BacktestEngine API Reference

The `BacktestEngine` class is the core simulation engine in **CryptoQuantLab**. It manages market data, order submission, execution, latency modeling, and portfolio state for one or more assets.

---

## Overview

`BacktestEngine` simulates a trading environment using historical market data and user-defined strategies. It supports realistic latency for order entry, response, and market data feeds, and provides access to portfolio statistics and order book state.

---

## Constructor
```cpp
BacktestEngine(const std::unordered_map<int, core::trading::AssetConfig>&asset_configs,
        const core::backtest::BacktestEngineConfig &engine_config,
        std::shared_ptr<utils::logger::Logger> logger = nullptr);
```

- **asset_configs**: Map of asset IDs to their configuration.
- **engine_config**: Simulation parameters (cash, latency, etc.).
- **logger**: Optional logger for debug and info output.

---

## Core Methods

### Simulation Control
```cpp
bool elapse(std::uint64_t us);
```
Advances the simulation clock by the specified microseconds, processing market events and delayed actions.

---

### Order Management
```cpp
OrderId submit_buy_order(int asset_id, Price price, Quantity quantity, TimeInForce tif, OrderType orderType);
OrderId submit_sell_order(int asset_id, Price price, Quantity quantity, TimeInForce tif, OrderType orderType);
void cancel_order(int asset_id, OrderId orderId);
void clear_inactive_orders();
```
- **submit_buy_order / submit_sell_order**: Submit new buy/sell orders with simulated latency.
- **cancel_order**: Cancel an active order.
- **clear_inactive_orders**: Remove filled, cancelled, or expired orders from the local state.

---

### Portfolio and State Access
- **orders**: Get all active orders for an asset.
- **cash**: Current cash balance.
- **equity**: Total portfolio value (cash + marked-to-market positions).
- **position**: Net position for an asset.
- **depth**: Current order book depth for an asset.
- **current_time**: Current simulation timestamp (microseconds).

---

### Latency Configuration
Set or get simulated latency for order entry, order response, and market data feed.

---

### Statistics and Reporting
```cpp
void print_trading_stats(int asset_id) const;
```
Prints trading statistics (number of trades, volume, value) for the specified asset.

---
## Notes

- All order submissions and cancellations are subject to configured latency.
- The engine supports multiple assets, each with independent order books and statistics.
- Use the logger for detailed event tracing and debugging.

---

## See Also

- [Configuration Guide](../configuration.md)
- [Strategies Guide](../strategies.md)
- [Usage Guide](../usage.md)

---