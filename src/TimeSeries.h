#pragma once
#include <string>
#include <vector>
#include "DataPoint.h"
#include "GorillaCompressor.h"

// A mechanical sympathy oriented contiguous array for a single metric
struct TimeSeries {
    std::string metric_name;
    std::vector<CompressedBlock> blocks;
    std::vector<DataPoint> active_block;
    
    static constexpr size_t BLOCK_THRESHOLD = 1000;

    void insert(int64_t timestamp, double value) {
        active_block.push_back({timestamp, value});
        if (active_block.size() >= BLOCK_THRESHOLD) {
            blocks.push_back(GorillaCompressor::compress(active_block));
            active_block.clear();
        }
    }

    std::vector<DataPoint> get_all_points() const {
        std::vector<DataPoint> all_points;
        for (const auto& block : blocks) {
            auto decompressed = GorillaCompressor::decompress(block);
            all_points.insert(all_points.end(), decompressed.begin(), decompressed.end());
        }
        all_points.insert(all_points.end(), active_block.begin(), active_block.end());
        return all_points;
    }

    std::vector<DataPoint> get_points_in_range(int64_t start_time, int64_t end_time) const {
        std::vector<DataPoint> result;
        for (const auto& block : blocks) {
            if (block.max_timestamp < start_time || block.min_timestamp > end_time) {
                continue;
            }
            auto decompressed = GorillaCompressor::decompress(block);
            for (const auto& dp : decompressed) {
                if (dp.timestamp >= start_time && dp.timestamp <= end_time) {
                    result.push_back(dp);
                }
            }
        }
        for (const auto& dp : active_block) {
            if (dp.timestamp >= start_time && dp.timestamp <= end_time) {
                result.push_back(dp);
            }
        }
        return result;
    }
};
