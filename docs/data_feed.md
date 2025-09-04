# WebSocket Stream Reader Usage

This document describes how to use the WebSocket stream reader classes in **CryptoQuantEngine** to connect to live market data sources, process messages, and handle threading.

---

## Overview

The WebSocket stream reader provides a base class (`BaseWebSocketStreamReader`) for connecting to a WebSocket URI, receiving messages, and processing them in a thread-safe manner. It is designed for extensibility and is used by concrete readers such as `BinanceStreamReader`.

---

## Basic Usage

### 1. Initialization

Create an instance of your stream reader (e.g., `BinanceStreamReader`) and provide the required URIs and output file paths:
```cpp
#include "core/market_data/readers/ws/binance_stream_reader.h"

const std::string ws_uri = "wss://fstream.binance.com/stream?streams=btcusdt@depth/btcusdt@trade"; 
const std::string rest_uri = "https://fapi.binance.com/fapi/v1/depth?symbol=BTCUSDT&limit=1000"; 
const std::string book_csv = "btcusdt_book.csv"; 
const std::string trade_csv = "btcusdt_trade.csv";
core::market_data::BinanceStreamReader reader(ws_uri, rest_uri, book_csv, trade_csv);
```


### 2. Connection Handling

- The reader automatically opens the WebSocket connection and starts background threads for message processing and CSV writing.
- Connection status can be checked via `is_connected()`.

### 3. Message Processing

- Incoming WebSocket messages are pushed to a thread-safe queue.
- The `process_queue` thread pops messages and calls `on_message(msg)`, which should be implemented in your derived class to parse and handle the data.

### 4. Threading Model

- The reader uses separate threads for:
  - WebSocket event loop
  - Message queue processing
  - CSV writing (for book/trade data)
  - Optional REST polling (for snapshots)

### 5. Graceful Shutdown

To stop the reader and clean up resources, call `disconnect()`:
```cpp
reader.disconnect();
```

---

## Example: Main Loop

```cpp
#include <csignal> 
#include <atomic> 
#include <thread> 
#include "core/market_data/readers/ws/binance_stream_reader.h"
std::atomic<bool> running{true}; 
void signal_handler(int) { running = false; }
int main(int argc, char *argv[]) { 
    std::signal(SIGINT, signal_handler);
    core::market_data::BinanceStreamReader reader(ws_uri, rest_uri, book_csv, trade_csv);
    while (running) {
        std::this_thread::sleep_for(std::chrono::seconds(1));
    }
    reader.disconnect();
    return 0;
}
```

---

## Extending the Reader

To implement a custom stream reader, inherit from `BaseWebSocketStreamReader` and override `on_message(const std::string &msg)` to parse and handle incoming data. The message structure will depend on the exchange's WebSocket API. 

---

## Further Reading

- [Data File Formats](data.md)
- [Configuration Guide](configuration.md)
- [Usage Guide](usage.md)