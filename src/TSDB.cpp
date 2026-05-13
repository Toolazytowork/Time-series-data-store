#include "TSDB.h"
#include <functional>
#include <mutex>

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
    ts.data.push_back({timestamp, value});
}

std::vector<DataPoint> TSDB::query(const std::string& metric_name) const {
    size_t idx = get_shard_idx(metric_name);
    const auto& shard = shards_[idx];

    std::shared_lock<std::shared_mutex> lock(shard.mutex);
    auto it = shard.store.find(metric_name);
    if (it != shard.store.end()) {
        return it->second.data;
    }
    return {};
}
