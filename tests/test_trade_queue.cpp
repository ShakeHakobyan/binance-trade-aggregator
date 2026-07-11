#include "trade_queue.h"
#include <chrono>
#include <gtest/gtest.h>
#include <thread>

TEST(TradeQueueTest, PushThenPopReturnsSameTrade) {
    TradeQueue queue;
    Trade trade;
    trade.symbol = "BTCUSDT";
    trade.price = 100.0;

    queue.push(trade);
    Trade result = queue.pop();

    EXPECT_EQ(result.symbol, "BTCUSDT");
    EXPECT_DOUBLE_EQ(result.price, 100.0);
}

TEST(TradeQueueTest, PopBlocksUntilPush) {
    TradeQueue queue;
    bool popped = false;

    std::thread consumer([&queue, &popped] {
        Trade t = queue.pop();
        popped = true;
    });

    std::this_thread::sleep_for(std::chrono::milliseconds(100));
    EXPECT_FALSE(popped);

    Trade trade;
    trade.symbol = "ETHUSDT";
    queue.push(trade);

    consumer.join();
    EXPECT_TRUE(popped);
}

TEST(TradeQueueTest, PreservesOrderFIFO) {
    TradeQueue queue;

    Trade first;
    first.symbol = "BTCUSDT";
    Trade second;
    second.symbol = "ETHUSDT";

    queue.push(first);
    queue.push(second);

    EXPECT_EQ(queue.pop().symbol, "BTCUSDT");
    EXPECT_EQ(queue.pop().symbol, "ETHUSDT");
}

TEST(TradeQueueTest, HandlesMultiplePushes) {
    TradeQueue queue;

    auto producer = [&queue](std::string symbol) {
        Trade trade;
        trade.symbol = symbol;
        queue.push(trade);
    };

    std::thread t1(producer, "BTCUSDT");
    std::thread t2(producer, "ETHUSDT");

    t1.join();
    t2.join();

    Trade a = queue.pop();
    Trade b = queue.pop();

    EXPECT_TRUE((a.symbol == "BTCUSDT" && b.symbol == "ETHUSDT") ||
                (a.symbol == "ETHUSDT" && b.symbol == "BTCUSDT"));
}
