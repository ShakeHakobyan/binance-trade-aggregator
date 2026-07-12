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
    auto result = queue.pop();

    ASSERT_TRUE(result.has_value());
    EXPECT_EQ(result->symbol, "BTCUSDT");
    EXPECT_DOUBLE_EQ(result->price, 100.0);
}

TEST(TradeQueueTest, PopBlocksUntilPush) {
    TradeQueue queue;
    bool popped = false;

    std::thread consumer([&queue, &popped] {
        auto t = queue.pop();
        popped = t.has_value();
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

    auto a = queue.pop();
    auto b = queue.pop();

    ASSERT_TRUE(a.has_value());
    ASSERT_TRUE(b.has_value());
    EXPECT_EQ(a->symbol, "BTCUSDT");
    EXPECT_EQ(b->symbol, "ETHUSDT");
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

    auto a = queue.pop();
    auto b = queue.pop();

    ASSERT_TRUE(a.has_value());
    ASSERT_TRUE(b.has_value());

    EXPECT_TRUE((a->symbol == "BTCUSDT" && b->symbol == "ETHUSDT") ||
                (a->symbol == "ETHUSDT" && b->symbol == "BTCUSDT"));
}

TEST(TradeQueueTest, PopReturnsNulloptAfterCloseOnEmptyQueue) {
    TradeQueue queue;
    queue.close();

    auto result = queue.pop();

    EXPECT_FALSE(result.has_value());
}

TEST(TradeQueueTest, CloseWakesBlockedPop) {
    TradeQueue queue;
    bool returned = false;
    bool gotValue = true;

    std::thread consumer([&queue, &returned, &gotValue] {
        auto t = queue.pop();
        returned = true;
        gotValue = t.has_value();
    });

    std::this_thread::sleep_for(std::chrono::milliseconds(100));
    EXPECT_FALSE(returned);

    queue.close();
    consumer.join();

    EXPECT_TRUE(returned);
    EXPECT_FALSE(gotValue);
}

TEST(TradeQueueTest, PushAfterCloseIsIgnored) {
    TradeQueue queue;
    queue.close();

    Trade trade;
    trade.symbol = "BTCUSDT";
    queue.push(trade);

    auto result = queue.pop();
    EXPECT_FALSE(result.has_value());
}
