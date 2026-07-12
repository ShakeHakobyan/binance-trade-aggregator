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

- `Trade` — plain struct: one parsed trade (symbol, price, quantity, exchange
  timestamp, isBuyerMaker).
- `TradeQueue` — thread-safe queue passing `Trade` objects from the network
  thread to the aggregation thread.
- `JsonTradeParser` — parses raw WebSocket messages into `Trade` objects;
  distinguishes subscription-ack control messages from actual trades.
- `BinanceClient` — connects to Binance combined stream endpoint (`/stream`),
  sends the `SUBSCRIBE` request for configured pairs, reads frames in a loop,
  and reconnects with exponential backoff on any failure (network drop,
  24h server-side disconnect, `serverShutdown` event). A fresh
  `io_context`/socket is created on every reconnect attempt to avoid reusing
  a socket left in an undefined state after a failure.
- `WindowStats` — per-(symbol, window) accumulator: trade count, volume,
  min/max price, buy/sell count. Updated incrementally via `update(Trade)`.
- `Aggregator` — owns `data_: map<symbol, unordered_map<windowStart, WindowStats>>`.
  `processTrade()` (private) buckets each trade by
  `windowStart = tradeTimeMs - (tradeTimeMs % aggregation_window_ms)` and
  updates the corresponding `WindowStats`. `extractClosedWindows(nowMs)`
  returns and removes all windows where `windowStart + aggregation_window_ms
  <= nowMs`.

## Testing

Unit tests use GoogleTest and cover `JsonTradeParser` and `TradeQueue`.

Run tests:

```bash
cd build
./unit_tests
```


## TODO

- [ ] `Config`.
- [ ] Unit tests for `FileWriter`.
- [ ] Graceful stop mechanism for all `run()` loops.
- [ ] Expand README with implementation nuances.
- [ ] Tests for the connection layer (`BinanceClient`).
- [ ] `systemd` unit file (`Restart=always`).
- [ ] Proper logging.
- [ ] Bound memory growth in `Aggregator::data_`.
- [ ] Explicitly detect and handle the `serverShutdown` event from Binance.
- [ ] Dockerfile.