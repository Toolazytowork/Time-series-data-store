#pragma once
#include <string>
#include <unordered_map>
#include <array>
#include <shared_mutex>
#include "TimeSeries.h"

// Core in-memory storage engine mapping metric names to TimeSeries
class TSDB {
public:
    TSDB() = default;
    ~TSDB() = default;

    // Inserts a single DataPoint into the specified metric's TimeSeries.
    void insert(const std::string& metric_name, int64_t timestamp, double value);

    // Retrieves all DataPoints for a given metric. Returns empty vector if not found.
    std::vector<DataPoint> query(const std::string& metric_name) const;

private:
    static constexpr size_t NUM_SHARDS = 16;

    struct Shard {
        std::unordered_map<std::string, TimeSeries> store;
        mutable std::shared_mutex mutex;
    };

    std::array<Shard, NUM_SHARDS> shards_;

    // Helper to compute shard index
    size_t get_shard_idx(const std::string& metric_name) const;
};
