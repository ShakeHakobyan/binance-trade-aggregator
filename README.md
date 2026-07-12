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

### Graceful shutdown

`SIGINT`/`SIGTERM` set a flag checked by the main loop.

- `Aggregator::stop()` closes `TradeQueue`, waking the blocked `pop()`
  inside `run()` (`pop()` returns `nullopt`, loop exits).
- `BinanceClient` — the pure, network-independent pieces: `buildSubscribeMessage()`
  (message format, including single-pair and empty-list edge cases) and
  `nextBackoffSeconds()` (backoff progression and cap). Socket-level logic
  (`connect`, `handshake`, `readLoop`) performs real network I/O and is
  verified manually, not by unit tests (see below).
- `main()` calls both `stop()`s, joins the threads, then does one final
  `extractClosedWindows()` + `write()` to flush any pending data.

**Known limitation:** shutdown can take up to ~2s (the read timeout) to
complete — an accepted trade-off for simplicity over async cancellation.

## Testing

Unit tests use GoogleTest and cover `JsonTradeParser` and `TradeQueue`.

Run tests:

```bash
cd build
./unit_tests
```

## Running as a systemd service

See `binance-service.service`. Deploy:

```bash
sudo cp build/binance_service /usr/local/bin/
sudo useradd --system --no-create-home binance-service
sudo mkdir -p /etc/binance-service
sudo cp config.json /etc/binance-service/
sudo chown -R binance-service:binance-service /etc/binance-service
sudo cp binance-service.service /etc/systemd/system/
sudo systemctl daemon-reload
sudo systemctl enable --now binance-service
```

`Restart=always` handles process crashes; `BinanceClient`'s internal
reconnect-with-backoff handles network-level disconnects — these are
complementary, not redundant.

## TODO

- [ ] Expand README with implementation nuances.
- [ ] Proper logging.
- [ ] Bound memory growth in `Aggregator::data_`.
- [ ] Explicitly detect and handle the `serverShutdown` event from Binance.
- [ ] Dockerfile.