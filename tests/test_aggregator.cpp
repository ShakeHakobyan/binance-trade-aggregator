#include "aggregator.h"
#include "trade_queue.h"
#include <gtest/gtest.h>

namespace {

Trade makeTrade(const std::string& symbol, double price, double qty, int64_t timeMs,
                bool isBuyerMaker) {
    Trade t;
    t.symbol = symbol;
    t.price = price;
    t.quantity = qty;
    t.tradeTimeMs = timeMs;
    t.isBuyerMaker = isBuyerMaker;
    return t;
}

void expectWindowsEqual(const StatsByTimeWindow& expected, const StatsByTimeWindow& actual) {
    ASSERT_EQ(expected.size(), actual.size());

    for (const auto& [windowStart, expectedSymbols] : expected) {
        ASSERT_TRUE(actual.count(windowStart) > 0) << "Missing window: " << windowStart;

        const auto& actualSymbols = actual.at(windowStart);
        ASSERT_EQ(expectedSymbols.size(), actualSymbols.size()) << "Window: " << windowStart;

        for (const auto& [symbol, expectedStats] : expectedSymbols) {
            ASSERT_TRUE(actualSymbols.count(symbol) > 0) << symbol << "@" << windowStart;

            const WindowStats& actualStats = actualSymbols.at(symbol);

            EXPECT_EQ(expectedStats.tradesNum, actualStats.tradesNum)
                << symbol << "@" << windowStart;
            EXPECT_DOUBLE_EQ(expectedStats.volume, actualStats.volume)
                << symbol << "@" << windowStart;
            EXPECT_DOUBLE_EQ(expectedStats.minPrice, actualStats.minPrice)
                << symbol << "@" << windowStart;
            EXPECT_DOUBLE_EQ(expectedStats.maxPrice, actualStats.maxPrice)
                << symbol << "@" << windowStart;
            EXPECT_EQ(expectedStats.buyCount, actualStats.buyCount) << symbol << "@" << windowStart;
            EXPECT_EQ(expectedStats.sellCount, actualStats.sellCount)
                << symbol << "@" << windowStart;
        }
    }
}

} // namespace

TEST(AggregatorTest, EmptyStoreReturnsNoWindows) {
    int64_t windowMs = 1000;

    TradeQueue queue;
    Aggregator agg(windowMs, queue);

    StatsByTimeWindow expected;

    auto actual = agg.extractClosedWindows(windowMs);

    expectWindowsEqual(expected, actual);
}

TEST(AggregatorTest, OpenWindowIsNotReturnedUntilClosed) {
    int64_t windowMs = 1000;
    int64_t windowStart = 0;
    int64_t timeJustBeforeClose = windowStart + windowMs - 1;
    int64_t timeAtClose = windowStart + windowMs;

    TradeQueue queue;
    Aggregator agg(windowMs, queue);
    agg.processTrade(makeTrade("BTCUSDT", 100.0, 1.0, windowStart, false));

    StatsByTimeWindow expectedStillOpen;
    auto actualStillOpen = agg.extractClosedWindows(timeJustBeforeClose);
    expectWindowsEqual(expectedStillOpen, actualStillOpen);

    StatsByTimeWindow expectedClosed;
    expectedClosed[windowStart]["BTCUSDT"] = WindowStats(1, 100.0, 100.0, 100.0, 1, 0);
    auto actualClosed = agg.extractClosedWindows(timeAtClose);
    expectWindowsEqual(expectedClosed, actualClosed);
}

TEST(AggregatorTest, MultipleTradesSameWindowAggregateCorrectly) {
    int64_t windowMs = 1000;
    int64_t windowStart = 0;

    double priceBuyOne = 100.0;
    double priceSell = 90.0;
    double priceBuyTwo = 110.0;
    double quantity = 1.0;
    double sellQuantity = 2.0;

    TradeQueue queue;
    Aggregator agg(windowMs, queue);
    agg.processTrade(makeTrade("BTCUSDT", priceBuyOne, quantity, 0, false));
    agg.processTrade(makeTrade("BTCUSDT", priceSell, sellQuantity, 500, true));
    agg.processTrade(makeTrade("BTCUSDT", priceBuyTwo, quantity, 999, false));

    double expectedVolume =
        priceBuyOne * quantity + priceSell * sellQuantity + priceBuyTwo * quantity;

    StatsByTimeWindow expected;
    expected[windowStart]["BTCUSDT"] = WindowStats(3, expectedVolume, priceSell, priceBuyTwo, 2, 1);

    auto actual = agg.extractClosedWindows(windowStart + windowMs);

    expectWindowsEqual(expected, actual);
}

TEST(AggregatorTest, TradesSplitAcrossDifferentWindows) {
    int64_t windowMs = 1000;
    int64_t firstWindowStart = 0;
    int64_t secondWindowStart = 1000;

    TradeQueue queue;
    Aggregator agg(windowMs, queue);
    agg.processTrade(makeTrade("BTCUSDT", 100.0, 1.0, 500, false));
    agg.processTrade(makeTrade("BTCUSDT", 200.0, 1.0, 1200, false));

    StatsByTimeWindow expected;
    expected[firstWindowStart]["BTCUSDT"] = WindowStats(1, 100.0, 100.0, 100.0, 1, 0);
    expected[secondWindowStart]["BTCUSDT"] = WindowStats(1, 200.0, 200.0, 200.0, 1, 0);

    auto actual = agg.extractClosedWindows(secondWindowStart + windowMs);

    expectWindowsEqual(expected, actual);
}

TEST(AggregatorTest, DifferentSymbolsAreIndependent) {
    int64_t windowMs = 1000;
    int64_t windowStart = 0;

    TradeQueue queue;
    Aggregator agg(windowMs, queue);
    agg.processTrade(makeTrade("BTCUSDT", 100.0, 1.0, windowStart, false));
    agg.processTrade(makeTrade("ETHUSDT", 50.0, 2.0, windowStart, true));

    StatsByTimeWindow expected;
    expected[windowStart]["BTCUSDT"] = WindowStats(1, 100.0, 100.0, 100.0, 1, 0);
    expected[windowStart]["ETHUSDT"] = WindowStats(1, 100.0, 50.0, 50.0, 0, 1);

    auto actual = agg.extractClosedWindows(windowStart + windowMs);

    expectWindowsEqual(expected, actual);
}

TEST(AggregatorTest, ExtractedWindowsAreRemovedFromStore) {
    int64_t windowMs = 1000;
    int64_t windowStart = 0;

    TradeQueue queue;
    Aggregator agg(windowMs, queue);
    agg.processTrade(makeTrade("BTCUSDT", 100.0, 1.0, windowStart, false));

    StatsByTimeWindow expectedFirst;
    expectedFirst[windowStart]["BTCUSDT"] = WindowStats(1, 100.0, 100.0, 100.0, 1, 0);
    auto actualFirst = agg.extractClosedWindows(windowStart + windowMs);
    expectWindowsEqual(expectedFirst, actualFirst);

    StatsByTimeWindow expectedSecond;
    auto actualSecond = agg.extractClosedWindows(windowStart + windowMs * 2);
    expectWindowsEqual(expectedSecond, actualSecond);
}

TEST(AggregatorTest, TradeExactlyOnWindowBoundaryGoesToNewWindow) {
    int64_t windowMs = 1000;
    int64_t boundaryTime = windowMs;

    TradeQueue queue;
    Aggregator agg(windowMs, queue);
    agg.processTrade(makeTrade("BTCUSDT", 100.0, 1.0, boundaryTime, false));

    StatsByTimeWindow expected;
    expected[boundaryTime]["BTCUSDT"] = WindowStats(1, 100.0, 100.0, 100.0, 1, 0);

    auto actual = agg.extractClosedWindows(boundaryTime + windowMs);

    expectWindowsEqual(expected, actual);
}

TEST(AggregatorTest, OnlyFullyClosedWindowsAreExtractedWhenMixed) {
    int64_t windowMs = 1000;
    int64_t firstWindowStart = 0;
    int64_t secondWindowStart = 1000;

    TradeQueue queue;
    Aggregator agg(windowMs, queue);
    agg.processTrade(makeTrade("BTCUSDT", 100.0, 1.0, firstWindowStart, false));
    agg.processTrade(makeTrade("BTCUSDT", 200.0, 1.0, secondWindowStart + 200, false));

    StatsByTimeWindow expected;
    expected[firstWindowStart]["BTCUSDT"] = WindowStats(1, 100.0, 100.0, 100.0, 1, 0);

    auto actual = agg.extractClosedWindows(firstWindowStart + windowMs);

    expectWindowsEqual(expected, actual);
}
