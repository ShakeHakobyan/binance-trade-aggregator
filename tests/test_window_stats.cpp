#include <gtest/gtest.h>

#include "window_stats.h"

TEST(WindowStatsTest, UpdatesSingleTrade) {
    WindowStats stats;

    Trade trade{.price = 100.0, .quantity = 2.0, .isBuyerMaker = true};

    stats.update(trade);

    EXPECT_EQ(stats.tradesNum, 1);
    EXPECT_DOUBLE_EQ(stats.volume, 200.0);
    EXPECT_DOUBLE_EQ(stats.minPrice, 100.0);
    EXPECT_DOUBLE_EQ(stats.maxPrice, 100.0);
    EXPECT_EQ(stats.buyCount, 0);
    EXPECT_EQ(stats.sellCount, 1);
}

TEST(WindowStatsTest, AggregatesMultipleTrades) {
    WindowStats stats;

    Trade buy{.price = 100.0, .quantity = 2.0, .isBuyerMaker = true};

    Trade sell{.price = 120.0, .quantity = 3.0, .isBuyerMaker = false};

    stats.update(buy);
    stats.update(sell);

    EXPECT_EQ(stats.tradesNum, 2);
    EXPECT_DOUBLE_EQ(stats.volume, 560.0);
    EXPECT_DOUBLE_EQ(stats.minPrice, 100.0);
    EXPECT_DOUBLE_EQ(stats.maxPrice, 120.0);
    EXPECT_EQ(stats.buyCount, 1);
    EXPECT_EQ(stats.sellCount, 1);
}

TEST(WindowStatsTest, UpdatesMinAndMaxPrice) {
    WindowStats stats;

    stats.update(Trade{.price = 200.0, .quantity = 1.0, .isBuyerMaker = true});

    stats.update(Trade{.price = 150.0, .quantity = 1.0, .isBuyerMaker = true});

    stats.update(Trade{.price = 250.0, .quantity = 1.0, .isBuyerMaker = false});

    EXPECT_DOUBLE_EQ(stats.minPrice, 150.0);
    EXPECT_DOUBLE_EQ(stats.maxPrice, 250.0);
}
