#pragma once
#include <vector>
#include <cstdint>
#include "DataPoint.h"

struct CompressedBlock {
    int64_t min_timestamp;
    int64_t max_timestamp;
    size_t count;
    std::vector<uint8_t> buffer;
};

class GorillaCompressor {
public:
    static CompressedBlock compress(const std::vector<DataPoint>& points);
    static std::vector<DataPoint> decompress(const CompressedBlock& block);
};
