# Configuration Guide

This document explains the configuration files used by **CryptoQuantLab**. Each config file is located in the `config/` directory and controls different aspects of the backtest engine, strategies, and output.

---

## 1. Asset Configuration (`asset_config.txt`)

Defines the properties of the trading asset and data sources.

**Parameters:**
- `book_update_file`: Path to the Level 2 order book CSV file.
- `trade_file`: Path to the trade data CSV file.
- `tick_size`: Minimum price increment for the asset.
- `lot_size`: Minimum tradeable quantity.
- `contract_multiplier`: Multiplier for contract value (usually 1.0 for spot).
- `is_inverse`: Set to `1` for inverse contracts, `0` otherwise.
- `maker_fee`: Fee rate for maker orders.
- `taker_fee`: Fee rate for taker orders.
- `name`: Asset name (optional, for reference).

---

## 2. Backtest Engine Configuration (`backtest_engine_config.txt`)

Sets simulation parameters for the backtest engine.

**Parameters:**
- `initial_cash`: Starting cash balance for the simulation.
- `order_entry_latency_us`: Latency (in microseconds) for order entry.
- `order_response_latency_us`: Latency (in microseconds) for order updates.
- `market_feed_latency_us`: Latency (in microseconds) for market data feed.

## 3. Recorder Configuration (`recorder_config.txt`)

Controls how performance metrics are recorded and output. 

**Parameters:**
- `interval_us`: Time interval (in microseconds) between equity snapshots.
- `output_file`: Output CSV file for recorded data.

---

## 4. Backtest Run Configuration (`backtest_config.txt`)

Controls the main backtest loop.

**Parameters:**
- `elapse_us`: Time to advance the simulation clock per iteration (in microseconds).
- `iterations`: Number of simulation iterations to run.

---

## Notes

- All config files use `key=value` format. Lines starting with `#` are comments.
- Paths should be relative to the project root or absolute.
- If a parameter is missing, a default value may be used (see source code for defaults).

---

## See Also

- [Getting Started](getting_started.md)
- [Usage Guide](usage.md)
- [Strategies](strategies.md)