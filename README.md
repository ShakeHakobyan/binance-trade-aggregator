# Binance Trade Aggregation Service

C++ service that connects to the Binance WebSocket API, subscribes to
multiple trade streams, aggregates trade statistics per symbol over
configurable time windows, and writes results to a file at a configurable
interval.

## Build

```bash
mkdir build && cd build
conan install .. --output-folder=. --build=missing -s build_type=Release
cmake .. -DCMAKE_TOOLCHAIN_FILE=conan_toolchain.cmake -DCMAKE_BUILD_TYPE=Release
cmake --build .
./binance_service ../config.json
```

Config path defaults to `config.json` in the current working directory if
no argument is given.

## Architecture

> Note: this section explains the *reasoning* behind the design. For usage, see Build/Testing sections.

- **`Trade`** — struct containing only the fields from the raw
  trade data that are actually needed for the statistics (symbol, price,
  quantity, exchange timestamp, buyer/seller flag).
- **`TradeQueue`** — thread-safe queue, network thread → aggregation
  thread, avoiding race conditions on push/pop. `pop()` returns
  `std::optional<Trade>` instead of `Trade` so a closed and drained queue
  can signal no more data (`nullopt`) to a thread blocked waiting on it.
- **`JsonTradeParser`** — split into `parseJson()`, `isServerShutdown()`,
  and `extractTrade()`. Split specifically so `BinanceClient` can check
  for `serverShutdown` before calling `extractTrade()` — otherwise it
  would just fail to find trade fields and log a misleading error.
  `serverShutdown` is sent by Binance before it closes the connection for
  planned maintenance, so it's handled as its own case, not as a parse
  failure.
- **`BinanceClient`** — owns the WebSocket connection: connect,
  subscribe, read loop, reconnect with growing delay on any failure.

  `connectAndListen()` creates a fresh `io_context` and socket on every
  attempt rather than reusing one stored on the class, so a failed
  connection never leaves a socket in an undefined state for the next
  retry.

  The read loop uses a socket-level receive timeout (`SO_RCVTIMEO`)
  instead of an unbounded blocking read. A timeout is not treated as an
  error, it just gives the loop a chance to check the stop flag; any
  other read error is treated as a real disconnect and triggers
  reconnect.

  A `serverShutdown` event is raised as an exception, so it's handled by
  the same reconnect path as an actual disconnect, rather than needing
  separate handling.

  `buildSubscribeMessage()` and `nextBackoffSeconds()` are standalone
  functions so they can be unit-tested without a real connection. The
  rest of the class performs real network I/O and is verified by running
  the service.
- **`WindowStats`** — per-window accumulator, updated incrementally via
  `update(Trade)`.
  In `update()`, `isBuyerMaker == true` increments `sellCount`
  and `false` increments `buyCount` — the reverse of what the field name
  suggests at a glance.
- **`Aggregator`** — trades come in from the queue one at a time. Each
  trade is not stored as-is. Instead, it immediately updates the
  statistics for its window and symbol (min/max/volume/counts), so memory
  use stays proportional to the number of windows in flight, not the
  number of trades received.

  How data is stored:

  ```
  windowStart (ms) -> { symbol -> WindowStats }

  1000 -> { "BTCUSDT": {trades: 12, volume: ..., ...},
            "ETHUSDT": {trades: 5,  volume: ..., ...} }
  2000 -> { "BTCUSDT": {trades: 8, ...} }
  ```

  Keyed by window first, symbol second, so a closed window comes out as
  one ready-to-write block (all its symbols together), matching what
  `FileWriter` needs.

  Each trade's window is found by rounding its exchange timestamp down to
  a fixed boundary (`tradeTimeMs - tradeTimeMs % windowMs`), so windows
  align to the epoch rather than to whenever the first trade happened to
  arrive.

  A window is closed once enough time has passed that no more trades for
  it are expected. Closed windows are moved out (not copied) since
  they're removed from storage right after.

  `stop()` just closes the queue it blocks on. `Aggregator` has no
  synchronization primitive of its own to wake up with.

- **`FileWriter`** — writes closed windows in the required output format.

  Symbols within a window are re-sorted into a `std::map` before writing,
  since the incoming data uses `unordered_map` and would otherwise print
  in a different, unstable order on every run.

  The `timestamp=` line for a window is only written if at least one
  symbol in it actually had trades — if every symbol in a window ended up
  empty, nothing is written for that window at all, matching the task's
  requirement to skip pairs with no trades.

  Writes with `std::ios::app`, so each call appends rather than
  overwriting previous output.
- **`Config`** — loads pairs, `aggregation_window_ms`,
  `serialization_interval_ms`, output path from JSON; throws on missing
  file, empty `pairs`, or non-positive intervals.

### Threading model

Three loops, synchronized only through `TradeQueue` and `Aggregator`'s
internal mutex — no other shared state:

1. **Network** (`BinanceClient::run`) — reads, parses, pushes to queue.
2. **Aggregation** (`Aggregator::run`) — pops continuously, buckets by
   *exchange* time.
3. **Serialization** (main thread) — every `serialization_interval_ms`
   (*wall-clock*), extracts closed windows and writes them.

`aggregation_window_ms` and `serialization_interval_ms` are independent by
design — the task specifies them as two separate config values, and
nothing in the code assumes they're equal.

### Failure handling

| Failure | Handling |
|---|---|
| WebSocket disconnect (network drop, 24h server limit) | Reconnect with exponential backoff, capped at 30s |
| `serverShutdown` event | Detected explicitly, triggers immediate reconnect rather than waiting for the drop |
| Missed ping/pong | Handled automatically by Boost.Beast at the protocol level |
| Malformed/unexpected JSON | Caught in `JsonTradeParser`, message dropped, service continues |
| File write failure (disk full, permissions) | Caught in `FileWriter`, logged, retried next interval |
| Config missing/invalid | Fails fast at startup, does not start with bad config |

## Log rotation

`binance-service.logrotate` rotates `output.txt` daily via `logrotate`,
keeping 7 days of history with `copytruncate` to avoid unbounded growth.

### Known limitations

- A trade whose window has already been extracted and written is dropped
  (with a log line) rather than silently reopening a closed window and
  producing a duplicate, incomplete `timestamp=` block on the next
  serialization cycle. Aggregator tracks the high-water mark of the last
  window it has closed and rejects anything at or before it.
- shutdown latency is bounded by the read timeout (~2s), not instant.
- `Aggregator`'s window map is unbounded — a very long network outage
  combined with a stalled serialization loop could grow memory
  indefinitely (not currently guarded against).

## Running as a systemd service

See `binance-service.service`.

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
reconnect handles network-level disconnects — these are complementary.

## Testing

```bash
cd build
./unit_tests
```

Covers `JsonTradeParser`, `TradeQueue` (including `close()`/wake
behavior), `Aggregator`, `WindowStats`, `FileWriter`, `Config`. For
`BinanceClient`, only the pure functions (`buildSubscribeMessage`,
`nextBackoffSeconds`) are unit-tested; the socket/reconnect logic
performs real network I/O and was verified manually by running the
service against the live endpoint and observing `output.txt`.

## TODO

- [ ] Structured logging (spdlog) instead of `std::cout`/`std::cerr`.
- [ ] Bound memory growth in `Aggregator` during prolonged outages.
- [ ] Dockerfile for reproducible builds.