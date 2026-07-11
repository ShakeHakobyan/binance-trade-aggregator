#pragma once
#include "trade.h"
#include "trade_queue.h"
#include "window_stats.h"

#include <atomic>
#include <cstdint>
#include <map>
#include <mutex>
#include <string>
#include <unordered_map>

using StatsBySymbolAndTimeWindow = std::map<std::string, std::unordered_map<int64_t, WindowStats>>;

class Aggregator {
  public:
    Aggregator(int64_t windowMs, TradeQueue &queue);

    void run();

    StatsBySymbolAndTimeWindow extractClosedWindows(int64_t nowMs);

    void onTrade(const Trade &trade);

  private:
    int64_t windowMs_;
    TradeQueue &queue_;

    std::mutex mutex_;
    StatsBySymbolAndTimeWindow data_;
};
