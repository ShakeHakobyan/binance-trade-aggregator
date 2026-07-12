#include "aggregator.h"
#include "binance_client.h"
#include "file_writer.h"
#include "trade_queue.h"

#include <chrono>
#include <iostream>
#include <string>
#include <thread>
#include <vector>

namespace {
// TODO: move these into config
const std::vector<std::string> pairs = {"btcusdt@trade", "ethusdt@trade", "bnbusdt@trade"};
constexpr int64_t aggregationWindowMs = 1000;
constexpr int64_t serializationIntervalMs = 1000;
const std::string outputPath = "output.txt";

int64_t nowMs() {
    return std::chrono::duration_cast<std::chrono::milliseconds>(
               std::chrono::system_clock::now().time_since_epoch())
        .count();
}
} // namespace

int main() {
    TradeQueue queue;
    BinanceClient client(pairs, queue);
    Aggregator aggregator(aggregationWindowMs, queue);
    FileWriter writer(outputPath);

    std::thread networkThread([&client] { client.run(); });

    std::thread aggregatorThread([&aggregator] { aggregator.run(); });

    std::cout << "Service started. Aggregation window: " << aggregationWindowMs
              << "ms, serialization interval: " << serializationIntervalMs << "ms.\n";

    while (true) {
        std::this_thread::sleep_for(std::chrono::milliseconds(serializationIntervalMs));

        auto closedWindows = aggregator.extractClosedWindows(nowMs());
        if (!writer.write(closedWindows)) {
            std::cerr << "[main] Failed to write output, will retry next interval.\n";
        }
    }

    networkThread.join();
    aggregatorThread.join();
    return 0;
}
