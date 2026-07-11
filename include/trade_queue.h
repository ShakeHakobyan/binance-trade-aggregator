#pragma once

#include "trade.h"
#include <condition_variable>
#include <mutex>
#include <queue>

class TradeQueue {
  public:
    void push(const Trade &trade);
    Trade pop();

  private:
    std::queue<Trade> queue_;
    std::mutex mutex_;
    std::condition_variable cv_;
};
