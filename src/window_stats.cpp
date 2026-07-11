#include "window_stats.h"

void WindowStats::update(const Trade &trade) {
    ++tradesNum;
    volume += trade.quantity * trade.price;
    minPrice = std::min(minPrice, trade.price);
    maxPrice = std::max(maxPrice, trade.price);
    buyCount += trade.isBuyerMaker;
    sellCount += !trade.isBuyerMaker;
}
