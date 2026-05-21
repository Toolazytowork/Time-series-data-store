#pragma once
#include "BitStreamReader.h"
#include <cstdint>

class GorillaValueDecompressor {
public:
    GorillaValueDecompressor(BitStreamReader& reader)
        : reader_(reader), is_first_(true), prev_val_(0),
          prev_leading_zeros_(32), prev_meaningful_length_(0) {}

    double read();

private:
    BitStreamReader& reader_;
    bool is_first_;
    uint64_t prev_val_;
    int prev_leading_zeros_;
    int prev_meaningful_length_;
};
