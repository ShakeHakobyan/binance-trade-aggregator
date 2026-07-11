#include "aggregator.h"

Aggregator::Aggregator(int64_t windowMs, TradeQueue &queue) : windowMs_(windowMs), queue_(queue) {
}

void Aggregator::processTrade(const Trade &trade) {
    int64_t windowStart = trade.tradeTimeMs - (trade.tradeTimeMs % windowMs_);

    std::lock_guard<std::mutex> lock(mutex_);
    data_[trade.symbol][windowStart].update(trade);
}

void Aggregator::run() {
    while (true) {
        Trade trade = queue_.pop();
        processTrade(trade);
    }
}

StatsBySymbolAndTimeWindow Aggregator::extractClosedWindows(int64_t nowMs) {
    StatsBySymbolAndTimeWindow closed;

    std::lock_guard<std::mutex> lock(mutex_);
    for (auto symbolIt = data_.begin(); symbolIt != data_.end();) {
        auto &windowsForSymbol = symbolIt->second;

        for (auto windowIt = windowsForSymbol.begin(); windowIt != windowsForSymbol.end();) {
            int64_t windowStart = windowIt->first;
            if (windowStart + windowMs_ <= nowMs) {
                closed[symbolIt->first][windowStart] = windowIt->second;
                windowIt = windowsForSymbol.erase(windowIt);
            } else {
                ++windowIt;
            }
        }

        if (windowsForSymbol.empty()) {
            symbolIt = data_.erase(symbolIt);
        } else {
            ++symbolIt;
        }
    }

    return closed;
}
