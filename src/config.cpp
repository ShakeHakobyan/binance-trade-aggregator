#include "config.h"
#include <fstream>
#include <stdexcept>

json Config::readJsonFile(const std::string& path) {
    std::ifstream in(path);
    if (!in.is_open()) {
        throw std::runtime_error("Config: cannot open file: " + path);
    }

    json parsed;
    try {
        in >> parsed;
    } catch (const std::exception& e) {
        throw std::runtime_error("Config: invalid JSON in " + path + ": " + e.what());
    }
    return parsed;
}

Config Config::parseConfig(const json& parsed) {
    Config config;
    config.pairs = parsed.at("pairs").get<std::vector<std::string>>();
    config.aggregationWindowMs = parsed.at("aggregation_window_ms").get<int64_t>();
    config.serializationIntervalMs = parsed.at("serialization_interval_ms").get<int64_t>();
    config.outputFilePath = parsed.value("output_file_path", "output.txt");
    return config;
}

void Config::validateConfig(const Config& config) {
    if (config.pairs.empty()) {
        throw std::runtime_error("Config: 'pairs' must not be empty");
    }
    if (config.aggregationWindowMs <= 0) {
        throw std::runtime_error("Config: 'aggregation_window_ms' must be positive");
    }
    if (config.serializationIntervalMs <= 0) {
        throw std::runtime_error("Config: 'serialization_interval_ms' must be positive");
    }
}

Config Config::loadFromFile(const std::string& path) {
    json parsed = readJsonFile(path);
    Config config = parseConfig(parsed);
    validateConfig(config);
    return config;
}
