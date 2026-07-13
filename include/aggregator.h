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

using WindowContents = std::unordered_map<std::string, WindowStats>;
using StatsByTimeWindow = std::map<int64_t, WindowContents>;

class Aggregator {
  public:
    Aggregator(int64_t windowMs, TradeQueue& queue);
    void processTrade(const Trade& trade);

    void run();
    void stop();
    StatsByTimeWindow extractClosedWindows(int64_t nowMs);

  private:
    bool isWindowClosed(int64_t windowStart, int64_t nowMs) const;

    int64_t windowMs_;
    TradeQueue& queue_;

    std::mutex mutex_;
    StatsByTimeWindow data_;
    int64_t lastClosedWindowStart_ = -1;
};
