#pragma once
#include "BitStreamWriter.h"
#include <cstdint>

class GorillaValueCompressor {
public:
    GorillaValueCompressor(BitStreamWriter& writer)
        : writer_(writer), is_first_(true), prev_val_(0),
          prev_leading_zeros_(32), prev_meaningful_length_(0) {}

    void append(double value);

private:
    BitStreamWriter& writer_;
    bool is_first_;
    uint64_t prev_val_;
    int prev_leading_zeros_;
    int prev_meaningful_length_;
};
