# CryptoQuantEngine

<img src="https://github.com/caped-doshi/cryptoquantengine/actions/workflows/ci.yml/badge.svg"> [![GitHub release](https://img.shields.io/github/v/release/caped-doshi/cryptoquantengine)](https://github.com/caped-doshi/cryptoquantengine/releases) [![License: MIT](https://img.shields.io/badge/License-MIT-yellow.svg)](LICENSE)

## High-Frequency Level 2 Trading Backtesting Engine

**CryptoQuantEngine** is a high-performance C++ framework for backtesting high-frequency trading (HFT) strategies on Level 2 crypto market data.

---

## Features

- **Full Depth-of-Book Simulation**  
  Accurately reconstructs and simulates the entire order book from Level 2 market data, supporting market-by-price feeds.

- **Realistic Latency Modeling**  
  Simulates order entry, order response, and market data feed latencies for robust evaluation of latency-sensitive strategies.

- **Queue Position Tracking**  
  Estimates queue position for passive orders, enabling realistic fill probability and execution modeling.

- **Flexible Order Matching Engine**  
  Provides configurable and extensible order matching logic for both market-making and liquidity-taking strategies.

- **Modular Strategy Framework**  
  Easily implement custom strategies by inheriting from the abstract strategy class and accessing market/order management APIs.

- **Comprehensive Performance Analytics**  
  Reports key metrics such as Sharpe ratio, Sortino ratio, max drawdown, and detailed trading statistics after each backtest run.

- **Automated Visualization**  
  Generates CSV outputs and Python-based plots for equity curves and position tracking.

- **Robust Unit Testing**  
  Includes a suite of unit tests to ensure reliability and correctness of core components.

---

## Quick Start

### Prerequisites

- C++20-compatible compiler 
- [CMake](https://cmake.org/) version 3.12 or later
- Python 3.7+ (for plotting)
- Python packages: `matplotlib`, `pandas`

### Build Instructions

Clone the repository
```bash
git clone https://github.com/caped-doshi/cryptoquantlab.git
```

Create and enter build directory
```bash
mkdir build && cd build
```

Configure the project with CMake
```bash
cmake -DCMAKE_BUILD_TYPE=Release ..
```

Compile the engine
```bash
cmake --build . --config Release
```

Run unit tests
```bash
ctest --output-on-failure
```

Run the backtest engine
```bash
./hftbacktest
```

---

This will:
- Load configuration files
- Run the backtest engine for the specified number of iterations
- Print final equity, performance metrics, and trading statistics
- Generate plots of equity and position (requires Python)

See [Usage Guide](docs/usage.md) for more details.

---

## Example Strategy: Grid Trading

A sample grid trading strategy is included.  
See [Examples](docs/examples.md) for implementation, configuration, and usage.

---

## Extending the Framework

To implement your own strategy:
- Inherit from the abstract `Strategy` class
- Implement `initialize()` and `on_elapse()`
- Use the `BacktestEngine` for market data and order management

See [Strategies Guide](docs/strategies.md) for instructions.

---

## Performance Metrics

After each run, the engine reports:
- **Sharpe Ratio** — risk-adjusted return
- **Sortino Ratio** — downside risk-adjusted return
- **Max Drawdown** — largest peak-to-trough loss
- **Trading statistics** — number of trades, volume, value

Plots are generated as CSV and visualized using Python.

---

## License

MIT License. See [LICENSE](LICENSE) for details.

---

## Further Reading

- [Configuration Guide](docs/configuration.md)
- [Usage Guide](docs/usage.md)
- [Strategies Guide](docs/strategies.md)
- [Examples](docs/examples.md)
- [API Reference](docs/api/)