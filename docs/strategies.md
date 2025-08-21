# Strategies Guide

This guide describes how strategies work in **CryptoQuantLab**, how to use the built-in grid trading strategy, and how to implement your own.

---

## Strategy Architecture

Strategies in CryptoQuantLab are implemented as C++ classes derived from the abstract `Strategy` base class:
```c++
class Strategy { 
  public: 
    virtual void initialize() = 0; 
    virtual void on_elapse(corebacktestBacktestEngine &hbt) = 0; 
    virtual ~Strategy() = default; 
};

```
- **initialize()**: Called once before the backtest loop starts.
- **on_elapse()**: Called on each simulation step, receives the backtest engine for order management and market data access.

---

## Built-in Strategy: Grid Trading

The framework includes a basic grid trading strategy, suitable for market making and liquidity provision.

### Overview

- Places buy and sell limit orders at multiple price levels around the mid price.
- Dynamically cancels and resubmits orders to maintain the grid.
- Respects position limits and notional order size.

### Configuration

Grid trading parameters are set in `config/grid_trading_config.txt`:
**Parameters:**
- `grid_num`: Number of grid levels on each side (bid/ask)
- `grid_interval`: Tick interval between grid levels
- `half_spread`: Half the spread in ticks for grid centering
- `position_limit`: Maximum allowed position size
- `notional_order_qty`: Notional value per grid order

### Usage

The grid trading strategy is instantiated and run in the main backtest loop:

```c++
core::strategy::GridTrading grid_trading(asset_id, grid_trading_config, logger);
while (engine.elapse(backtest_config.elapse_us) && iter-- > 0) { 
    engine.clear_inactive_orders();
    grid_trading.on_elapse(engine); 
    recorder.record(engine, asset_id); 
}
```
---

## Implementing Custom Strategies

To add your own strategy:

1. **Create a new class** in `core/strategy/` that inherits from `Strategy`.
2. **Implement** the `initialize()` and `on_elapse()` methods.
3. **Use the BacktestEngine** in `on_elapse()` to access market data and submit/cancel orders.

**Example Skeleton:**
```c++
#include "core/strategy/strategy.h"

void on_elapse(core::backtest::BacktestEngine &engine) override {
    // Access market data
    auto mid_price = engine.get_mid_price(asset_id);
    
    // Submit orders, manage positions, etc.
    if (/* some condition */) {
        engine.submit_order(/* parameters */);
    }
}
```
4. **Instantiate your strategy** in the main backtest file and call its methods in the simulation loop.

---

## Strategy Testing

- Add unit tests for your strategy in `tests/strategies/`.
- Use the provided Catch2 framework for automated testing.

---

## Tips

- Use the logger for debugging and tracking strategy actions.
- Tune grid and risk parameters in the config files for optimal results.
- Review the [examples.md](examples.md) for sample strategy implementations.

---

## Further Reading

- [Configuration Guide](configuration.md)
- [Usage Guide](usage.md)
- [Examples](examples.md)
- [FAQ](faq.md)

---