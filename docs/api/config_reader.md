# ConfigReader API Reference

The `ConfigReader` class in **CryptoQuantLab** is responsible for loading, parsing, and providing access to configuration parameters from text files. It supports reading key-value pairs, type conversion, and structured config objects for all major modules.

---

## Overview

`ConfigReader` loads configuration files in `key=value` format, parses values as strings, doubles, or integers, and provides methods to retrieve typed values and structured configs for assets, strategies, engine, recorder, and backtest runs.

---

## Constructor
```cpp
ConfigReader();
```
Creates a new config reader instance.o

---

## Core Methods

### Loading and Clearing
```cpp
void load(const std::string &filename);
```
Loads and parses a config files and obtains the key-value pairs.
```cpp
void clear();
```
Clears all loaded key-value pairs.

---

## Core Methods

### Structured Config Retrieval

```cpp
core::trading::AssetConfig get_asset_config(const std::string &filename);
core::backtest::BacktestConfig get_backtest_config(const std::string &filename);
core::recorder::RecorderConfig get_recorder_config(const std::string &filename);
core::backtest::BacktestEngineConfig get_engine_config(const std::string &filename);
core::strategy::GridTradingConfig get_grid_trading_config(const std::string &filename);
```
These methods load and return structured configuration objects, more details can be found in the [Configuration Guide](../configuration.md). 

---

## Notes

- Config files must use `key=value` format; lines starting with `#` are comments.
- Missing required keys will throw exceptions.
- Some structured config methods provide default values if keys are missing (see source for details).
- All values are stored as strings internally and converted on access.

---

## See Also

- [Recorder API](recorder.md)
- [BacktestEngine API](backtest_engine.md)
- [Configuration Guide](../configuration.md)

---