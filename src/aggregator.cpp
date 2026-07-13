#include "aggregator.h"
#include <iostream>

Aggregator::Aggregator(int64_t windowMs, TradeQueue& queue) : windowMs_(windowMs), queue_(queue) {
}

void Aggregator::processTrade(const Trade& trade) {
    int64_t windowStart = trade.tradeTimeMs - (trade.tradeTimeMs % windowMs_);

    std::lock_guard<std::mutex> lock(mutex_);

    if (windowStart <= lastClosedWindowStart_) {
        std::cerr << "[Aggregator] Dropping late trade for " << trade.symbol << " (window "
                  << windowStart << " already closed, last closed " << lastClosedWindowStart_
                  << ")\n";
        return;
    }

    data_[windowStart][trade.symbol].update(trade);
}

void Aggregator::run() {
    while (true) {
        auto trade = queue_.pop();
        if (!trade.has_value()) {
            break;
        }
        processTrade(*trade);
    }
}

void Aggregator::stop() {
    queue_.close();
}

bool Aggregator::isWindowClosed(int64_t windowStart, int64_t nowMs) const {
    return windowStart + windowMs_ <= nowMs;
}

StatsByTimeWindow Aggregator::extractClosedWindows(int64_t nowMs) {
    StatsByTimeWindow closed;

    std::lock_guard<std::mutex> lock(mutex_);
    for (auto windowIt = data_.begin(); windowIt != data_.end();) {
        int64_t windowStart = windowIt->first;

        if (isWindowClosed(windowStart, nowMs)) {
            closed[windowStart] = std::move(windowIt->second);
            if (windowStart > lastClosedWindowStart_) {
                lastClosedWindowStart_ = windowStart;
            }
            windowIt = data_.erase(windowIt);
        } else {
            ++windowIt;
        }
    }

    return closed;
}
