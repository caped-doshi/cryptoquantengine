# hftengine

## High-Frequency Level 2 Trading Backtesting Engine

`hftengine` is a high-performance C++ framework for backtesting high-frequency trading (HFT) strategies on **Level 2 crypto market data**. It supports **full order book reconstruction**, **queue position tracking**, and **realistic order matching** with configurable latency models.

This engine is designed for research and development of market-making and latency-sensitive strategies on tick-level data.

---

## Key Features

- **C++20 implementation** for speed and low-level control.
- **Full depth-of-book simulation** from market-by-price Level 2 feeds.
- **Feed and order latency simulation** for realistic execution timing.
- **Queue position estimation** for passive order fills.
- **Modular testable architecture** with Catch2 unit tests.

---

## Quick Start

### Prerequisites

- C++20-compatible compiler (GCC >= 10, Clang >= 11, MSVC >= 19.28)
- [CMake](https://cmake.org/) version 3.12 or later

### Build Instructions

```bash
# Clone the repository
git clone https://github.com/your_username/hftengine.git
cd hftengine

# Create and enter build directory
mkdir build
cd build

# Configure the build
cmake ..

# Compile the engine (Debug build)
cmake --build . --config Debug

# Run tests inside the build directory
ctest

# Run the engine inside the build directory
./hftengine