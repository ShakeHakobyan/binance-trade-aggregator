#include "trade_queue.h"

void TradeQueue::push(const Trade& trade) {
    std::lock_guard<std::mutex> lock(mutex_);
    if (closed_) {
        return;
    }
    queue_.push(trade);
    cv_.notify_one();
}

std::optional<Trade> TradeQueue::pop() {
    std::unique_lock<std::mutex> lock(mutex_);
    cv_.wait(lock, [this] { return !queue_.empty() || closed_; });

    if (queue_.empty()) {
        return std::nullopt;
    }
    Trade trade = queue_.front();
    queue_.pop();
    return trade;
}

void TradeQueue::close() {
    std::lock_guard<std::mutex> lock(mutex_);
    closed_ = true;
    cv_.notify_all();
}
