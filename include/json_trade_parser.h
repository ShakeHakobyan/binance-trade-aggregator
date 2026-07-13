#pragma once

#include "trade.h"
#include <nlohmann/json.hpp>
#include <optional>
#include <string>

using json = nlohmann::json;

class JsonTradeParser {
  public:
    static std::optional<json> parseJson(const std::string& rawMessage);
    static bool extractTrade(const json& data, Trade& outTrade);
    static bool parse(const std::string& rawMessage, Trade& outTrade);
    static bool isServerShutdown(const json& data);
};
