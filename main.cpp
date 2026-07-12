#include "aggregator.h"
#include "binance_client.h"
#include "config.h"
#include "file_writer.h"
#include "trade_queue.h"

#include <atomic>
#include <chrono>
#include <csignal>
#include <iostream>
#include <thread>

namespace {

std::atomic<bool> g_shutdownRequested{false};

void handleSignal(int) {
    g_shutdownRequested = true;
}

int64_t nowMs() {
    return std::chrono::duration_cast<std::chrono::milliseconds>(
               std::chrono::system_clock::now().time_since_epoch())
        .count();
}
} // namespace

int main(int argc, char* argv[]) {
    std::signal(SIGINT, handleSignal);
    std::signal(SIGTERM, handleSignal);

    std::string configPath = (argc > 1) ? argv[1] : "config.json";

    Config config;
    try {
        config = Config::loadFromFile(configPath);
    } catch (const std::exception& e) {
        std::cerr << "[main] Failed to load config: " << e.what() << "\n";
        return 1;
    }

    TradeQueue queue;
    BinanceClient client(config.pairs, queue);
    Aggregator aggregator(config.aggregationWindowMs, queue);
    FileWriter writer(config.outputFilePath);

    std::thread networkThread([&client] { client.run(); });
    std::thread aggregatorThread([&aggregator] { aggregator.run(); });

    std::cout << "Service started. Aggregation window: " << config.aggregationWindowMs
              << "ms, serialization interval: " << config.serializationIntervalMs
              << "ms. Press Ctrl+C to stop.\n";

    while (!g_shutdownRequested) {
        std::this_thread::sleep_for(std::chrono::milliseconds(config.serializationIntervalMs));

        auto closedWindows = aggregator.extractClosedWindows(nowMs());
        if (!writer.write(closedWindows)) {
            std::cerr << "[main] Failed to write output, will retry next interval.\n";
        }
    }

    std::cout << "[main] Shutdown requested, stopping components...\n";
    client.stop();
    aggregator.stop();

    networkThread.join();
    aggregatorThread.join();

    auto finalWindows = aggregator.extractClosedWindows(nowMs());
    writer.write(finalWindows);

    std::cout << "[main] Stopped cleanly.\n";
    return 0;
}
