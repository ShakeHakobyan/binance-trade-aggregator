#include "trade_queue.h"

void TradeQueue::push(const Trade &trade) {
    std::lock_guard<std::mutex> lock(mutex_);
    queue_.push(trade);
    cv_.notify_one();
}

Trade TradeQueue::pop() {
    std::unique_lock<std::mutex> lock(mutex_);
    cv_.wait(lock, [this] { return !queue_.empty(); });
    Trade trade = queue_.front();
    queue_.pop();
    return trade;
}