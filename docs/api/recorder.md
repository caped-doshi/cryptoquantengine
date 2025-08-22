# Recorder API Reference

The `Recorder` class in **CryptoQuantLab** is responsible for tracking portfolio performance, recording equity and position snapshots, calculating risk metrics, and generating output for analysis and plotting.

---

## Overview

`Recorder` collects time-series data during backtests, computes performance statistics (Sharpe, Sortino, max drawdown), and can output results to CSV and plots. It is typically used in the main simulation loop to record state at each step.

---

## Constructor
```cpp
Recorder(Microseconds interval_us, std::shared_ptr<utils::logger::Logger> logger = nullptr);
```
- **interval_us**: Time interval (in microseconds) between equity snapshots.
- **logger**: Optional logger for debug and info output.

---

## Core Methods

### Recording State
```cpp
void record(const EquitySnapshot& equity_snapshot);
```
**record(EquitySnapshot)**: Add a custom equity snapshot.
```cpp
void record(Timestamp timestamp, double equity);
```
**record(Timestamp, double)**: Record equity at a specific timestamp.
```cpp
void record(const BacktestEngine &engine, int asset_id);
```
**record(const BacktestEngine&, int)**: Record equity and position for a specific asset in the backtest engine.

---

### Performance Metrics
```cpp
double sharpe() const;
```
**sharpe()**: Returns the annualized Sharpe ratio. 
```cpp
double sortino() const;
```
**sortino()**: Returns the annualized Sortino ratio.
```cpp
double max_drawdown() const;
```
**max_drawdown()**: Returns the maximum drawdown as a decimal in (0.0, 1.0).

### Results Output
```cpp
void print_performance_metrics() const;
```
Prints Sharpe, Sortino, and max drawdown to the console.
```cpp
void plot(int asset_id) const;
```
Generates a CSV and calls a Python script to plot equity, position, and mid price for the specified asset.

---

## Example Usage
```cpp
core::recorder::Recorder recorder(1'000'000, logger); // 1 second interval
while (engine.elapse(100'000)) {
    // Simulate some strategy logic here
    recorder.record_state(engine, asset_id);
} 
```
---

## Notes

- The `interval_us` parameter controls the granularity of recorded data and risk calculations.
- Plots require Python 3 and the `matplotlib`/`pandas` packages.
- Output CSV files are named `recorder_plot_<asset_id>.csv` by default.

---

## See Also

- [BacktestEngine API](backtest_engine.md)
- [Configuration Guide](../configuration.md)
- [Usage Guide](../usage.md)

---