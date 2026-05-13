#pragma once
#include <string>
#include <unordered_map>
#include <unordered_set>
#include <shared_mutex>
#include <mutex>
#include <vector>

// Inverted Index mapping tag_key:tag_value to metric names
class TagIndex {
public:
    TagIndex() = default;
    ~TagIndex() = default;

    // Adds a metric to the inverted index for the given tags
    void add_metric_tags(const std::string& metric_name, const std::unordered_map<std::string, std::string>& tags) {
        if (tags.empty()) return;
        
        std::unique_lock<std::shared_mutex> lock(mutex_);
        for (const auto& [key, value] : tags) {
            std::string tag_kv = key + ":" + value;
            index_[tag_kv].insert(metric_name);
        }
    }

    // Retrieves metrics matching a single tag_key:tag_value
    std::unordered_set<std::string> get_metrics_by_tag(const std::string& tag_key, const std::string& tag_value) const {
        std::shared_lock<std::shared_mutex> lock(mutex_);
        std::string tag_kv = tag_key + ":" + tag_value;
        auto it = index_.find(tag_kv);
        if (it != index_.end()) {
            return it->second;
        }
        return {};
    }

private:
    std::unordered_map<std::string, std::unordered_set<std::string>> index_;
    mutable std::shared_mutex mutex_;
};
