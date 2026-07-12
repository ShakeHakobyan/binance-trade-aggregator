#include "file_writer.h"

#include <ctime>
#include <fstream>
#include <iomanip>
#include <iostream>
#include <map>
#include <sstream>

FileWriter::FileWriter(std::string outputPath) : outputPath_(std::move(outputPath)) {
}

std::string FileWriter::formatTimestamp(int64_t windowStartMs) {
    std::time_t seconds = static_cast<std::time_t>(windowStartMs / 1000);
    std::tm utcTime{};
    gmtime_r(&seconds, &utcTime);

    std::ostringstream oss;
    oss << std::put_time(&utcTime, "%Y-%m-%dT%H:%M:%SZ");
    return oss.str();
}

std::string FileWriter::formatSymbolLine(const std::string &symbol, const WindowStats &stats) {
    std::ostringstream oss;
    oss << "symbol=" << symbol << " trades=" << stats.tradesNum << " volume=" << stats.volume
        << " min=" << stats.minPrice << " max=" << stats.maxPrice << " buy=" << stats.buyCount
        << " sell=" << stats.sellCount;
    return oss.str();
}

bool FileWriter::write(const StatsByTimeWindow &closedWindows) {
    if (closedWindows.empty()) {
        return true;
    }

    std::ofstream out(outputPath_, std::ios::app);
    if (!out.is_open()) {
        std::cerr << "[FileWriter] Failed to open output file: " << outputPath_ << "\n";
        return false;
    }

    for (const auto &windowEntry : closedWindows) {
        int64_t windowStart = windowEntry.first;

        std::map<std::string, WindowStats> sortedSymbols(windowEntry.second.begin(),
                                                         windowEntry.second.end());

        bool wroteTimestamp = false;
        for (const auto &symbolEntry : sortedSymbols) {
            const WindowStats &stats = symbolEntry.second;
            if (stats.tradesNum == 0) {
                continue;
            }

            if (!wroteTimestamp) {
                out << "timestamp=" << formatTimestamp(windowStart) << "\n";
                wroteTimestamp = true;
            }
            out << formatSymbolLine(symbolEntry.first, stats) << "\n";
        }
    }

    out.flush();
    if (!out) {
        std::cerr << "[FileWriter] Write error (disk full / I/O failure) on: " << outputPath_
                  << "\n";
        return false;
    }

    return true;
}
