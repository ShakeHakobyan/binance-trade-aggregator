#include "json_trade_parser.h"
#include <gtest/gtest.h>

TEST(JsonTradeParserTest, ParsesDifferentValidTrade) {
    std::string raw = R"({
        "stream": "ethusdt@trade",
        "data": {
            "e": "trade",
            "s": "ETHUSDT",
            "p": "2500.55",
            "q": "10.25",
            "T": 1736699999999,
            "m": true
        }
    })";

    Trade trade;
    bool ok = JsonTradeParser::parse(raw, trade);

    ASSERT_TRUE(ok);
    EXPECT_EQ(trade.symbol, "ETHUSDT");
    EXPECT_DOUBLE_EQ(trade.price, 2500.55);
    EXPECT_DOUBLE_EQ(trade.quantity, 10.25);
    EXPECT_EQ(trade.tradeTimeMs, 1736699999999);
    EXPECT_TRUE(trade.isBuyerMaker);
}

TEST(JsonTradeParserTest, RejectsMissingSymbol) {
    std::string raw = R"({
        "data": {
            "p": "100",
            "q": "1",
            "T": 123456,
            "m": false
        }
    })";

    Trade trade;
    bool ok = JsonTradeParser::parse(raw, trade);
    EXPECT_FALSE(ok);
}

TEST(JsonTradeParserTest, RejectsMissingPrice) {
    std::string raw = R"({
        "data": {
            "s": "BTCUSDT",
            "q": "1",
            "T": 123456,
            "m": false
        }
    })";

    Trade trade;
    bool ok = JsonTradeParser::parse(raw, trade);
    EXPECT_FALSE(ok);
}

TEST(JsonTradeParserTest, RejectsMissingQuantity) {
    std::string raw = R"({
        "data": {
            "s": "BTCUSDT",
            "p": "50000",
            "T": 123456,
            "m": false
        }
    })";

    Trade trade;
    bool ok = JsonTradeParser::parse(raw, trade);
    EXPECT_FALSE(ok);
}

TEST(JsonTradeParserTest, RejectsMissingTimestamp) {
    std::string raw = R"({
        "data": {
            "s": "BTCUSDT",
            "p": "50000",
            "q": "0.1",
            "m": false
        }
    })";

    Trade trade;
    bool ok = JsonTradeParser::parse(raw, trade);
    EXPECT_FALSE(ok);
}

TEST(JsonTradeParserTest, RejectsWrongPriceType) {
    std::string raw = R"({
        "data": {
            "s": "BTCUSDT",
            "p": {},
            "q": "0.1",
            "T": 123456,
            "m": false
        }
    })";

    Trade trade;
    bool ok = JsonTradeParser::parse(raw, trade);
    EXPECT_FALSE(ok);
}

TEST(JsonTradeParserTest, RejectsWrongBooleanType) {
    std::string raw = R"({
        "data": {
            "s": "BTCUSDT",
            "p": "100",
            "q": "1",
            "T": 123456,
            "m": "false"
        }
    })";

    Trade trade;
    bool ok = JsonTradeParser::parse(raw, trade);
    EXPECT_FALSE(ok);
}

TEST(JsonTradeParserTest, RejectsEmptyDataObject) {
    std::string raw = R"({
        "data": {}
    })";

    Trade trade;
    bool ok = JsonTradeParser::parse(raw, trade);
    EXPECT_FALSE(ok);
}

TEST(JsonTradeParserTest, RejectsEmptyMessage) {
    std::string raw = "";

    Trade trade;
    bool ok = JsonTradeParser::parse(raw, trade);
    EXPECT_FALSE(ok);
}

TEST(JsonTradeParserTest, RejectsNullData) {
    std::string raw = R"({
        "data": null
    })";

    Trade trade;
    bool ok = JsonTradeParser::parse(raw, trade);
    EXPECT_FALSE(ok);
}

TEST(JsonTradeParserTest, ParsesVerySmallQuantity) {
    std::string raw = R"({
        "data": {
            "s": "BTCUSDT",
            "p": "43000.1",
            "q": "0.00000001",
            "T": 1,
            "m": false
        }
    })";

    Trade trade;

    bool ok = JsonTradeParser::parse(raw, trade);
    ASSERT_TRUE(ok);
    EXPECT_DOUBLE_EQ(trade.quantity, 0.00000001);
}
