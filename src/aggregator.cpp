#include "aggregator.h"

Aggregator::Aggregator(int64_t windowMs, TradeQueue &queue) : windowMs_(windowMs), queue_(queue) {
}

void Aggregator::processTrade(const Trade &trade) {
    int64_t windowStart = trade.tradeTimeMs - (trade.tradeTimeMs % windowMs_);

    std::lock_guard<std::mutex> lock(mutex_);
    data_[windowStart][trade.symbol].update(trade);
}

void Aggregator::run() {
    while (true) {
        Trade trade = queue_.pop();
        processTrade(trade);
    }
}

StatsByTimeWindow Aggregator::extractClosedWindows(int64_t nowMs) {
    StatsByTimeWindow closed;

    std::lock_guard<std::mutex> lock(mutex_);
    for (auto windowIt = data_.begin(); windowIt != data_.end();) {
        int64_t windowStart = windowIt->first;

        if (windowStart + windowMs_ <= nowMs) {
            closed[windowStart] = std::move(windowIt->second);
            windowIt = data_.erase(windowIt);
        } else {
            ++windowIt;
        }
    }

    return closed;
}
