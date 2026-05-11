#include "TSDB.h"
#include <functional>

size_t TSDB::get_shard_idx(const std::string& metric_name) const {
    return std::hash<std::string>{}(metric_name) % NUM_SHARDS;
}

void TSDB::insert(const std::string& metric_name, int64_t timestamp, double value) {
    size_t idx = get_shard_idx(metric_name);
    auto& store = shards_[idx].store;

    auto& ts = store[metric_name];
    if (ts.metric_name.empty()) {
        ts.metric_name = metric_name;
    }
    ts.data.push_back({timestamp, value});
}

std::vector<DataPoint> TSDB::query(const std::string& metric_name) const {
    size_t idx = get_shard_idx(metric_name);
    const auto& store = shards_[idx].store;

    auto it = store.find(metric_name);
    if (it != store.end()) {
        return it->second.data;
    }
    return {};
}
