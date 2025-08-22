## Asset Data File Formats

### 1. Book Update File (CSV)

**Required columns (header row):**
- `timestamp`: Exchange timestamp (integer, microseconds since epoch)
- `local_timestamp`: Local timestamp (integer, microseconds since epoch; if missing, will be computed)
- `is_snapshot`: `true` or `false` (string; indicates if the row is a full snapshot or incremental update)
- `side`: `bid` or `ask` (string)
- `price`: Price level (float)
- `amount`: Quantity at price level (float; `0` means delete level)

**Example:**
```plaintext
timestamp,local_timestamp,is_snapshot,side,price,amount
1740009604700000,1740009604703670,false,ask,2.7347,4.2 
1740009604840000,1740009604859720,false,bid,2.7346,76.8
```

---

### 2. Trade File (CSV)

**Required columns (header row):**

- `timestamp`: Exchange timestamp (integer, microseconds since epoch)
- `local_timestamp`: Local timestamp (integer, microseconds since epoch; if missing, will be computed)
- `id`: Trade/order ID (integer)
- `side`: `buy` or `sell` (string)
- `price`: Trade price (float)
- `amount`: Trade quantity (float)

**Example:**
```plaintext
timestamp,local_timestamp,id,side,price,amount 
1740009604700000,1740009604703670,47311612,buy,2.7347,4.2 
1740009604840000,1740009604859720,47311613,sell,2.7346,76.8
```


**Notes:**
- If `local_timestamp` is missing, it will be set to `timestamp + market_feed_latency_us` by the engine.
- All files must have a header row matching the required columns (order does not matter).
- All timestamps are in microseconds.

---

**Tip:**  
See the `tests/market_data/test_trade_stream_reader.cpp` and `tests/strategies/test_grid_trading.cpp` for example file generation and usage.

---
## Further Reading

- [Configuration Guide](configuration.md)
- [Usage Guide](usage.md)
- [Strategies Guide](strategies.md)
- [Examples](examples.md)
- [API Reference](api/)
