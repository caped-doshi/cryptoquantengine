# Usage Guide

This guide explains how to run backtests, interpret results, and use the main features of **CryptoQuantEngine**.

---

## Running a Backtest

After building the project, run the main executable from the build directory:

```bash
./hftbacktest
```

This will:
- Load configuration files from the `config/` directory
- Run the backtest engine for the specified number of iterations
- Print final equity, performance metrics, and trading statistics
- Generate a plot of equity and position (requires Python)

---

## Command-Line Options

Currently, the main executable does **not** accept command-line arguments.  
All settings are controlled via configuration files in the `config/` directory.

---

## Output

After a successful run, you will see:
- **Backtest wall time** (total runtime in seconds)
- **Final equity** (portfolio value at the end of the simulation)
- **Performance metrics**:
  - Sharpe Ratio
  - Sortino Ratio
  - Max Drawdown
- **Trading statistics** for each asset:
  - Number of trades
  - Trading volume
  - Trading value

If plotting is enabled, a CSV file and plot will be generated for each asset.

---

## Example Workflow

1. Edit configuration files in `config/` as needed.
2. Build the project.
3. Run the backtest.

---

## Custom Strategies

To implement or modify strategies:
- Edit or add strategy classes in `core/strategy/`
- Update configuration files as needed
- Rebuild and rerun the backtest

See [strategies.md](strategies.md) for more details.

---

## Troubleshooting

- Ensure all required config files are present in `config/`
- Check for missing or invalid data files referenced in configs
- If plotting fails, verify Python and required packages are installed

---

## Further Reading

- [Getting Started](getting_started.md)
- [Configuration Guide](configuration.md)
- [Strategies](strategies.md)
- [Examples](examples.md)
- [FAQ](faq.md)

---