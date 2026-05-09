#pragma once
#include <string>
#include <unordered_map>
#include "TimeSeries.h"

// Core in-memory storage engine mapping metric names to TimeSeries
class TSDB {
public:
    TSDB() = default;
    ~TSDB() = default;

private:
    std::unordered_map<std::string, TimeSeries> store_;
};
