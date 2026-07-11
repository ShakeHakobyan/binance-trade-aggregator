#pragma once

#include "trade.h"
#include <cstdint>

struct WindowStats {
    uint64_t tradesNum = 0;
    double volume = 0.0;
    double minPrice = 0.0;
    double maxPrice = 0.0;
    uint64_t buyCount = 0;
    uint64_t sellCount = 0;

    void update(const Trade &trade);
};
