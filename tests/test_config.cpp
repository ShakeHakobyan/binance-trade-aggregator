#include "config.h"
#include <gtest/gtest.h>
#include <fstream>
#include <cstdio>

namespace {

std::string writeTempConfig(const std::string& contents, const std::string& path) {
    std::ofstream out(path);
    out << contents;
    return path;
}

} // namespace

TEST(ConfigTest, LoadsValidConfig) {
    std::string path = "test_config_valid.json";
    writeTempConfig(R"({
        "pairs": ["btcusdt@trade", "ethusdt@trade"],
        "aggregation_window_ms": 500,
        "serialization_interval_ms": 2000,
        "output_file_path": "custom_output.txt"
    })", path);

    Config config = Config::loadFromFile(path);

    EXPECT_EQ(config.pairs.size(), 2u);
    EXPECT_EQ(config.pairs[0], "btcusdt@trade");
    EXPECT_EQ(config.aggregationWindowMs, 500);
    EXPECT_EQ(config.serializationIntervalMs, 2000);
    EXPECT_EQ(config.outputFilePath, "custom_output.txt");

    std::remove(path.c_str());
}

TEST(ConfigTest, UsesDefaultOutputPathIfMissing) {
    std::string path = "test_config_default_output.json";
    writeTempConfig(R"({
        "pairs": ["btcusdt@trade"],
        "aggregation_window_ms": 1000,
        "serialization_interval_ms": 1000
    })", path);

    Config config = Config::loadFromFile(path);

    EXPECT_EQ(config.outputFilePath, "output.txt");
    std::remove(path.c_str());
}

TEST(ConfigTest, ThrowsOnMissingFile) {
    EXPECT_THROW(Config::loadFromFile("does_not_exist.json"), std::runtime_error);
}

TEST(ConfigTest, ThrowsOnEmptyPairs) {
    std::string path = "test_config_empty_pairs.json";
    writeTempConfig(R"({
        "pairs": [],
        "aggregation_window_ms": 1000,
        "serialization_interval_ms": 1000
    })", path);

    EXPECT_THROW(Config::loadFromFile(path), std::runtime_error);
    std::remove(path.c_str());
}

TEST(ConfigTest, ThrowsOnNonPositiveInterval) {
    std::string path = "test_config_bad_interval.json";
    writeTempConfig(R"({
        "pairs": ["btcusdt@trade"],
        "aggregation_window_ms": 0,
        "serialization_interval_ms": 1000
    })", path);

    EXPECT_THROW(Config::loadFromFile(path), std::runtime_error);
    std::remove(path.c_str());
}