#pragma once

#include "trade.h"
#include <condition_variable>
#include <mutex>
#include <optional>
#include <queue>

class TradeQueue {
  public:
    void push(const Trade& trade);
    std::optional<Trade> pop();
    void close();

  private:
    std::queue<Trade> queue_;
    std::mutex mutex_;
    std::condition_variable cv_;
    bool closed_ = false;
};
