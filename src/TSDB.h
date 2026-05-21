#pragma once
#include <string>
#include <unordered_map>
#include <array>
#include <shared_mutex>
#include <functional>
#include "TimeSeries.h"
#include "TagIndex.h"

// Core in-memory storage engine mapping metric names to TimeSeries
class TSDB {
public:
    TSDB() = default;
    ~TSDB() = default;

    // Inserts a single DataPoint into the specified metric's TimeSeries.
    void insert(const std::string& metric_name, int64_t timestamp, double value, const std::unordered_map<std::string, std::string>& tags = {});

    // Retrieves all DataPoints for a given metric. Returns empty vector if not found.
    std::vector<DataPoint> query(const std::string& metric_name) const;

    // Complex query: filters metrics by tags (AND logic), and returns data points within [start_time, end_time]
    std::unordered_map<std::string, std::vector<DataPoint>> query_complex(
        const std::unordered_map<std::string, std::string>& tags,
        int64_t start_time,
        int64_t end_time) const;

    // Executes a callback for every DataPoint in the database. Used for snapshotting.
    void dump_all(const std::function<void(const std::string&, const DataPoint&)>& callback) const;

    const TagIndex& get_tag_index() const { return tag_index_; }

private:
    static constexpr size_t NUM_SHARDS = 16;

    struct alignas(64) Shard {
        std::unordered_map<std::string, TimeSeries> store;
        mutable std::shared_mutex mutex;
    };

    std::array<Shard, NUM_SHARDS> shards_;
    TagIndex tag_index_;

    // Helper to compute shard index
    size_t get_shard_idx(const std::string& metric_name) const;
};
