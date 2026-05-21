#include "GorillaCompressor.h"
#include "BitStreamWriter.h"
#include "BitStreamReader.h"
#include "GorillaTimestamp.h"
#include "GorillaValueCompressor.h"
#include "GorillaValueDecompressor.h"

CompressedBlock GorillaCompressor::compress(const std::vector<DataPoint>& points) {
    CompressedBlock block;
    if (points.empty()) {
        block.min_timestamp = 0;
        block.max_timestamp = 0;
        block.count = 0;
        return block;
    }

    block.min_timestamp = points.front().timestamp;
    block.max_timestamp = points.back().timestamp;
    block.count = points.size();

    BitStreamWriter writer;
    GorillaTimestampCompressor ts_comp(writer);
    GorillaValueCompressor val_comp(writer);

    for (const auto& dp : points) {
        ts_comp.append(dp.timestamp);
        val_comp.append(dp.value);
    }

    block.buffer = writer.get_buffer();
    return block;
}

std::vector<DataPoint> GorillaCompressor::decompress(const CompressedBlock& block) {
    std::vector<DataPoint> points;
    if (block.count == 0) {
        return points;
    }

    points.reserve(block.count);
    BitStreamReader reader(block.buffer);
    GorillaTimestampDecompressor ts_decomp(reader);
    GorillaValueDecompressor val_decomp(reader);

    for (size_t i = 0; i < block.count; ++i) {
        int64_t ts = ts_decomp.read();
        double val = val_decomp.read();
        points.push_back({ts, val});
    }

    return points;
}
