#include "binance_client.h"
#include "trade_queue.h"
#include "trade.h"

#include <iostream>
#include <thread>
#include <vector>
#include <string>

int main() {
    std::vector<std::string> pairs = {
        "btcusdt@trade",
        "ethusdt@trade",
        "bnbusdt@trade"
    };

    TradeQueue queue;
    BinanceClient client(pairs, queue);
    
    std::thread networkThread([&client] {
        client.run();
    });

    while (true) {
        Trade trade = queue.pop();
        std::cout << "[" << trade.symbol << "] "
                  << "price=" << trade.price
                  << " qty=" << trade.quantity
                  << " time=" << trade.tradeTimeMs
                  << " buyerIsMaker=" << trade.isBuyerMaker
                  << std::endl;
    }

    networkThread.join();
    return 0;
}