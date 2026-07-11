#pragma once

#include "trade.h"

#include <cstdint>
#include <limits>

struct WindowStats {
    uint64_t tradesNum = 0;
    double volume = 0.0;

    double minPrice = std::numeric_limits<double>::max();
    double maxPrice = std::numeric_limits<double>::lowest();

    uint64_t buyCount = 0;
    uint64_t sellCount = 0;

    void update(const Trade &trade);
};
