#include "file_writer.h"
#include <gtest/gtest.h>
#include <fstream>
#include <sstream>
#include <cstdio>

namespace {

std::string readFile(const std::string& path) {
    std::ifstream in(path);
    std::ostringstream ss;
    ss << in.rdbuf();
    return ss.str();
}

} // namespace

class FileWriterTest : public ::testing::Test {
  protected:
    std::string path = "test_output_file_writer.txt";

    void TearDown() override {
        std::remove(path.c_str());
    }
};

TEST_F(FileWriterTest, WritesSingleWindowSingleSymbol) {
    FileWriter writer(path);

    StatsByTimeWindow closed;
    closed[0]["BTCUSDT"] = WindowStats(2, 300.0, 100.0, 200.0, 1, 1);

    ASSERT_TRUE(writer.write(closed));

    std::string content = readFile(path);
    EXPECT_NE(content.find("timestamp=1970-01-01T00:00:00Z"), std::string::npos);
    EXPECT_NE(content.find("symbol=BTCUSDT trades=2 volume=300 min=100 max=200 buy=1 sell=1"),
              std::string::npos);
}

TEST_F(FileWriterTest, GroupsMultipleSymbolsUnderSameTimestamp) {
    FileWriter writer(path);

    StatsByTimeWindow closed;
    closed[0]["BTCUSDT"] = WindowStats(1, 100.0, 100.0, 100.0, 1, 0);
    closed[0]["ETHUSDT"] = WindowStats(1, 50.0, 50.0, 50.0, 0, 1);

    ASSERT_TRUE(writer.write(closed));

    std::string content = readFile(path);
    size_t timestampCount = 0;
    size_t pos = 0;
    while ((pos = content.find("timestamp=", pos)) != std::string::npos) {
        ++timestampCount;
        ++pos;
    }

    EXPECT_EQ(timestampCount, 1u);
    EXPECT_NE(content.find("symbol=BTCUSDT"), std::string::npos);
    EXPECT_NE(content.find("symbol=ETHUSDT"), std::string::npos);
}

TEST_F(FileWriterTest, SkipsSymbolsWithZeroTrades) {
    FileWriter writer(path);

    StatsByTimeWindow closed;
    closed[0]["BTCUSDT"] = WindowStats(0, 0.0, 0.0, 0.0, 0, 0);
    closed[0]["ETHUSDT"] = WindowStats(1, 50.0, 50.0, 50.0, 1, 0);

    ASSERT_TRUE(writer.write(closed));

    std::string content = readFile(path);
    EXPECT_EQ(content.find("BTCUSDT"), std::string::npos);
    EXPECT_NE(content.find("ETHUSDT"), std::string::npos);
}

TEST_F(FileWriterTest, SkipsTimestampEntirelyIfAllSymbolsEmpty) {
    FileWriter writer(path);

    StatsByTimeWindow closed;
    closed[0]["BTCUSDT"] = WindowStats(0, 0.0, 0.0, 0.0, 0, 0);

    ASSERT_TRUE(writer.write(closed));

    std::string content = readFile(path);
    EXPECT_TRUE(content.empty());
}

TEST_F(FileWriterTest, EmptyInputProducesNoWriteAndNoError) {
    FileWriter writer(path);

    StatsByTimeWindow closed;

    ASSERT_TRUE(writer.write(closed));
}

TEST_F(FileWriterTest, AppendsAcrossMultipleWriteCalls) {
    FileWriter writer(path);

    StatsByTimeWindow first;
    first[0]["BTCUSDT"] = WindowStats(1, 100.0, 100.0, 100.0, 1, 0);
    ASSERT_TRUE(writer.write(first));

    StatsByTimeWindow second;
    second[1000]["BTCUSDT"] = WindowStats(1, 200.0, 200.0, 200.0, 1, 0);
    ASSERT_TRUE(writer.write(second));

    std::string content = readFile(path);
    EXPECT_NE(content.find("1970-01-01T00:00:00Z"), std::string::npos);
    EXPECT_NE(content.find("1970-01-01T00:00:01Z"), std::string::npos);
}

TEST_F(FileWriterTest, ReturnsFalseOnUnwritableDirectory) {
    FileWriter writer("/nonexistent_dir_xyz/output.txt");

    StatsByTimeWindow closed;
    closed[0]["BTCUSDT"] = WindowStats(1, 100.0, 100.0, 100.0, 1, 0);

    EXPECT_FALSE(writer.write(closed));
}