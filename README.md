# CryptoQuantLab

## High-Frequency Level 2 Trading Backtesting Engine

**CryptoQuantLab** is a high-performance C++ framework for backtesting high-frequency trading (HFT) strategies on Level 2 crypto market data.    
It supports full order book reconstruction, queue position tracking, and realistic order matching with configurable latency models.

This engine is designed for research and development of market-making, liquidity-taking, and latency-sensitive strategies on orderbook data.

---

## Features

- **C++20 implementation** for speed and low-level control.
- **Full depth-of-book simulation** from market-by-price Level 2 feeds.
- **Configurable latency simulation** for order management and market data.
- **Queue position estimation** for passive order fills.
- **Modular, extensible architecture** for custom strategies.
- **Comprehensive performance metrics** (Sharpe, Sortino, max drawdown).
- **Automated plotting** of equity and position (Python required).
- **Unit tests** with Catch2 for reliability.

---

## Quick Start

### Prerequisites

- C++20-compatible compiler (GCC ? 10, Clang ? 11, MSVC ? 19.28)
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
- [FAQ](docs/faq.md)

