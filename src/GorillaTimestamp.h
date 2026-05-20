#pragma once
#include "BitStreamWriter.h"
#include "BitStreamReader.h"
#include <cstdint>

class GorillaTimestampCompressor {
public:
    GorillaTimestampCompressor(BitStreamWriter& writer) 
        : writer_(writer), is_first_(true), prev_timestamp_(0), prev_delta_(0) {}

    void append(int64_t timestamp);

private:
    BitStreamWriter& writer_;
    bool is_first_;
    int64_t prev_timestamp_;
    int64_t prev_delta_;
};

class GorillaTimestampDecompressor {
public:
    GorillaTimestampDecompressor(BitStreamReader& reader)
        : reader_(reader), is_first_(true), prev_timestamp_(0), prev_delta_(0) {}

    int64_t read();

private:
    BitStreamReader& reader_;
    bool is_first_;
    int64_t prev_timestamp_;
    int64_t prev_delta_;
};
