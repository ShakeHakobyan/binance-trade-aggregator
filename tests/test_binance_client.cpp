#include "binance_client.h"
#include <gtest/gtest.h>

TEST(BinanceClientTest, BuildsCorrectSubscribeMessage) {
    std::vector<std::string> pairs = {"btcusdt@trade", "ethusdt@trade"};
    std::string msg = BinanceClient::buildSubscribeMessage(pairs, 1);

    EXPECT_NE(msg.find("\"method\":\"SUBSCRIBE\""), std::string::npos);
    EXPECT_NE(msg.find("btcusdt@trade"), std::string::npos);
    EXPECT_NE(msg.find("ethusdt@trade"), std::string::npos);
    EXPECT_NE(msg.find("\"id\":1"), std::string::npos);
}

TEST(BinanceClientTest, SubscribeMessageHandlesSinglePair) {
    std::vector<std::string> pairs = {"btcusdt@trade"};
    std::string msg = BinanceClient::buildSubscribeMessage(pairs, 42);

    EXPECT_NE(msg.find("btcusdt@trade"), std::string::npos);
    EXPECT_NE(msg.find("\"id\":42"), std::string::npos);
}

TEST(BinanceClientTest, SubscribeMessageHandlesEmptyPairs) {
    std::vector<std::string> pairs;
    std::string msg = BinanceClient::buildSubscribeMessage(pairs, 1);

    EXPECT_NE(msg.find("\"params\":[]"), std::string::npos);
}

TEST(BinanceClientTest, BackoffDoublesEachTime) {
    EXPECT_EQ(BinanceClient::nextBackoffSeconds(1, 30), 2);
    EXPECT_EQ(BinanceClient::nextBackoffSeconds(2, 30), 4);
    EXPECT_EQ(BinanceClient::nextBackoffSeconds(4, 30), 8);
    EXPECT_EQ(BinanceClient::nextBackoffSeconds(8, 30), 16);
}

TEST(BinanceClientTest, BackoffCapsAtMax) {
    EXPECT_EQ(BinanceClient::nextBackoffSeconds(16, 30), 30);
    EXPECT_EQ(BinanceClient::nextBackoffSeconds(30, 30), 30);
}

TEST(BinanceClientTest, BackoffStartsFromOne) {
    EXPECT_EQ(BinanceClient::nextBackoffSeconds(1, 30), 2);
}
