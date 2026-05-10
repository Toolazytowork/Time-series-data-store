#pragma once
#include <string>
#include <unordered_map>
#include "TimeSeries.h"

// Core in-memory storage engine mapping metric names to TimeSeries
class TSDB {
public:
    TSDB() = default;
    ~TSDB() = default;

    // Inserts a single DataPoint into the specified metric's TimeSeries.
    void insert(const std::string& metric_name, int64_t timestamp, double value) {
        auto& ts = store_[metric_name];
        if (ts.metric_name.empty()) {
            ts.metric_name = metric_name;
        }
        ts.data.push_back({timestamp, value});
    }

    // Retrieves all DataPoints for a given metric. Returns empty vector if not found.
    std::vector<DataPoint> query(const std::string& metric_name) const {
        auto it = store_.find(metric_name);
        if (it != store_.end()) {
            return it->second.data;
        }
        return {};
    }

private:
    std::unordered_map<std::string, TimeSeries> store_;
};
