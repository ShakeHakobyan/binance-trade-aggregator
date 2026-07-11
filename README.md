# Binance Trade Aggregation Service

C++ service that connects to the Binance WebSocket API, subscribes to
multiple trade streams, aggregates trade statistics per symbol over
configurable time windows, and writes results to a file at a configurable
interval.

## Build

```bash
mkdir build && cd build
conan install .. --output-folder=. --build=missing
cmake .. -DCMAKE_TOOLCHAIN_FILE=conan_toolchain.cmake
cmake --build .
./binance_service
```

## Architecture

- `BinanceClient` — connects to Binance combined WebSocket stream, subscribes
  to configured pairs, handles reconnection with backoff on any failure.
- `JsonTradeParser` — parses raw WebSocket messages into `Trade` objects.
- `TradeQueue` — thread-safe queue passing `Trade` objects from the network
  thread to the aggregation thread.