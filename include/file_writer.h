#pragma once

#include "aggregator.h"
#include "window_stats.h"

#include <cstdint>
#include <string>

class FileWriter {
  public:
    explicit FileWriter(std::string outputPath);

    bool write(const StatsByTimeWindow& closedWindows);

  private:
    static std::string formatTimestamp(int64_t windowStartMs);
    static std::string formatSymbolLine(const std::string& symbol, const WindowStats& stats);

    std::string outputPath_;
};
