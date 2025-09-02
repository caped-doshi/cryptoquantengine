
## Planned Features
---
### Backtesting Engine
---
- [ ] **Live Market Data Recording**  
  Implement a module to connect to live market data streams via websockets (e.g., trades, incremental book updates). Received data will be written to CSV files in a format compatible with the backtest engine. This will include robust error handling, automatic reconnection logic, and comprehensive documentation.

- [ ] **Support for Inverse Perpetuals**  
  Extend the framework to support inverse perpetual contracts, enabling accurate backtesting and strategy development for these instrument types.
---
### Options
---
- [ ] **Data Handling**  
  Develop functionality to ingest and process options market data, including bid/ask prices, implied volatility, and Greeks.
- [ ] **Backtesting Framework**  
  Adapt the existing backtesting engine to accommodate options trading strategies, including handling of options-specific order types and execution logic.
- [ ] **Pricing Models**  
  Integrate standard options pricing models (e.g., Black-Scholes, Binomial Trees) to facilitate options strategy backtesting.