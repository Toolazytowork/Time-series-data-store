#pragma once
#include <string>
#include <vector>
#include "DataPoint.h"

// A mechanical sympathy oriented contiguous array for a single metric
struct TimeSeries {
    std::string metric_name;
    std::vector<DataPoint> data;
};
