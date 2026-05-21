#include "GorillaValueDecompressor.h"
#include <cstring>

static inline double uint64_to_double(uint64_t u) {
    double d;
    std::memcpy(&d, &u, sizeof(double));
    return d;
}

double GorillaValueDecompressor::read() {
    if (is_first_) {
        prev_val_ = reader_.read_bits(64);
        is_first_ = false;
        return uint64_to_double(prev_val_);
    }

    if (reader_.read_bit() == 0) {
        return uint64_to_double(prev_val_);
    }

    uint64_t xor_val = 0;
    if (reader_.read_bit() == 0) {
        uint64_t bits = reader_.read_bits(prev_meaningful_length_);
        xor_val = bits << (64 - prev_leading_zeros_ - prev_meaningful_length_);
    } else {
        int leading_zeros = reader_.read_bits(5);
        int meaningful_length = reader_.read_bits(6) + 1;
        uint64_t bits = reader_.read_bits(meaningful_length);
        int trailing_zeros = 64 - leading_zeros - meaningful_length;
        xor_val = bits << trailing_zeros;

        prev_leading_zeros_ = leading_zeros;
        prev_meaningful_length_ = meaningful_length;
    }

    uint64_t val_bits = prev_val_ ^ xor_val;
    prev_val_ = val_bits;
    return uint64_to_double(val_bits);
}
