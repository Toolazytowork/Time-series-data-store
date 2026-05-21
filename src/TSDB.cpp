#include "TSDB.h"
#include <functional>
#include <mutex>
#include <algorithm>

size_t TSDB::get_shard_idx(const std::string& metric_name) const {
    return std::hash<std::string>{}(metric_name) % NUM_SHARDS;
}

void TSDB::insert(const std::string& metric_name, int64_t timestamp, double value, const std::unordered_map<std::string, std::string>& tags) {
    size_t idx = get_shard_idx(metric_name);
    auto& shard = shards_[idx];

    std::unique_lock<std::shared_mutex> lock(shard.mutex);
    auto& ts = shard.store[metric_name];
    if (ts.metric_name.empty()) {
        ts.metric_name = metric_name;
        tag_index_.add_metric_tags(metric_name, tags);
    }
    ts.insert(timestamp, value);
}

std::vector<DataPoint> TSDB::query(const std::string& metric_name) const {
    size_t idx = get_shard_idx(metric_name);
    const auto& shard = shards_[idx];

    std::shared_lock<std::shared_mutex> lock(shard.mutex);
    auto it = shard.store.find(metric_name);
    if (it != shard.store.end()) {
        return it->second.get_all_points();
    }
    return {};
}

std::unordered_map<std::string, std::vector<DataPoint>> TSDB::query_complex(
    const std::unordered_map<std::string, std::string>& tags,
    int64_t start_time,
    int64_t end_time) const {

    std::unordered_map<std::string, std::vector<DataPoint>> result;
    if (tags.empty()) return result;

    // 1. Resolve tags using inverted index
    std::vector<std::unordered_set<std::string>> sets;
    for (const auto& [key, value] : tags) {
        auto metrics = tag_index_.get_metrics_by_tag(key, value);
        if (metrics.empty()) return result; // AND logic means if any tag returns empty, result is empty
        sets.push_back(std::move(metrics));
    }

    // 2. Compute intersection (AND logic)
    std::unordered_set<std::string> intersection = std::move(sets.front());
    for (size_t i = 1; i < sets.size(); ++i) {
        std::unordered_set<std::string> current_intersection;
        for (const auto& metric : intersection) {
            if (sets[i].count(metric)) {
                current_intersection.insert(metric);
            }
        }
        intersection = std::move(current_intersection);
        if (intersection.empty()) return result;
    }

    // 3. For each metric in intersection, fetch points in time range
    for (const auto& metric_name : intersection) {
        size_t idx = get_shard_idx(metric_name);
        const auto& shard = shards_[idx];

        std::shared_lock<std::shared_mutex> lock(shard.mutex);
        auto it = shard.store.find(metric_name);
        if (it != shard.store.end()) {
            std::vector<DataPoint> data = it->second.get_points_in_range(start_time, end_time);
            if (!data.empty()) {
                result[metric_name] = std::move(data);
            }
        }
    }

    return result;
}

void TSDB::dump_all(const std::function<void(const std::string&, const DataPoint&)>& callback) const {
    for (size_t i = 0; i < NUM_SHARDS; ++i) {
        const auto& shard = shards_[i];
        std::shared_lock<std::shared_mutex> lock(shard.mutex);
        for (const auto& [metric_name, series] : shard.store) {
            for (const auto& dp : series.get_all_points()) {
                callback(metric_name, dp);
            }
        }
    }
}
