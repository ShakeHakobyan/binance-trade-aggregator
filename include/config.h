#pragma once

#include <cstdint>
#include <nlohmann/json.hpp>
#include <string>
#include <vector>

using json = nlohmann::json;

class Config {
  public:
    std::vector<std::string> pairs;
    int64_t aggregationWindowMs = 1000;
    int64_t serializationIntervalMs = 1000;
    std::string outputFilePath = "output.txt";

    static Config loadFromFile(const std::string& path);

  private:
    static json readJsonFile(const std::string& path);
    static Config parseConfig(const json& parsed);
    static void validateConfig(const Config& config);
};
