#include "json_trade_parser.h"

#include <iostream>

std::optional<json> JsonTradeParser::parseJson(const std::string& rawMessage) {
    try {
        json parsed = json::parse(rawMessage);
        if (!parsed.contains("data")) {
            std::cout << "[control message] " << rawMessage << std::endl;
            return std::nullopt;
        }
        return parsed["data"];
    } catch (const std::exception& e) {
        std::cerr << "Failed to parse JSON: " << e.what() << std::endl;
        return std::nullopt;
    }
}

bool JsonTradeParser::isServerShutdown(const json& data) {
    return data.contains("e") && data["e"] == "serverShutdown";
}

bool JsonTradeParser::extractTrade(const json& data, Trade& outTrade) {
    try {
        outTrade.symbol = data.at("s").get<std::string>();
        outTrade.price = std::stod(data.at("p").get<std::string>());
        outTrade.quantity = std::stod(data.at("q").get<std::string>());
        outTrade.tradeTimeMs = data.at("T").get<int64_t>();
        outTrade.isBuyerMaker = data.at("m").get<bool>();
        return true;
    } catch (const std::exception& e) {
        std::cerr << "Failed to extract trade: " << e.what() << std::endl;
        return false;
    }
}

bool JsonTradeParser::parse(const std::string& rawMessage, Trade& outTrade) {
    auto dataOpt = JsonTradeParser::parseJson(rawMessage);

    if (!dataOpt) {
        return false;
    }

    if (JsonTradeParser::isServerShutdown(*dataOpt)) {
        return false;
    }

    return JsonTradeParser::extractTrade(*dataOpt, outTrade);
}
