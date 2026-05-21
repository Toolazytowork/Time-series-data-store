#include "GorillaValueCompressor.h"
#include <cstring>
#include <algorithm>

#ifdef _MSC_VER
#include <intrin.h>
#endif

static inline int count_leading_zeros(uint64_t val) {
    if (val == 0) return 64;
#if defined(__GNUC__) || defined(__clang__)
    return __builtin_clzll(val);
#else
    unsigned long index;
    _BitScanReverse64(&index, val);
    return 63 - index;
#endif
}

static inline int count_trailing_zeros(uint64_t val) {
    if (val == 0) return 64;
#if defined(__GNUC__) || defined(__clang__)
    return __builtin_ctzll(val);
#else
    unsigned long index;
    _BitScanForward64(&index, val);
    return index;
#endif
}

static inline uint64_t double_to_uint64(double d) {
    uint64_t u;
    std::memcpy(&u, &d, sizeof(double));
    return u;
}

void GorillaValueCompressor::append(double value) {
    uint64_t val_bits = double_to_uint64(value);
    if (is_first_) {
        writer_.write_bits(val_bits, 64);
        prev_val_ = val_bits;
        is_first_ = false;
        return;
    }

    uint64_t xor_val = val_bits ^ prev_val_;
    if (xor_val == 0) {
        writer_.write_bit(false); // 0
    } else {
        writer_.write_bit(true); // 1
        
        int clz = count_leading_zeros(xor_val);
        int ctz = count_trailing_zeros(xor_val);
        
        int leading_zeros = std::min(clz, 31);
        int trailing_zeros = ctz;
        int meaningful_length = 64 - leading_zeros - trailing_zeros;

        if (prev_meaningful_length_ > 0 && 
            leading_zeros >= prev_leading_zeros_ && 
            trailing_zeros >= (64 - prev_leading_zeros_ - prev_meaningful_length_)) {
            
            writer_.write_bit(false); // 0 (reuse window)
            uint64_t bits_to_write = xor_val >> (64 - prev_leading_zeros_ - prev_meaningful_length_);
            writer_.write_bits(bits_to_write, prev_meaningful_length_);
        } else {
            writer_.write_bit(true); // 1 (new window)
            writer_.write_bits(leading_zeros, 5);
            writer_.write_bits(meaningful_length - 1, 6);
            
            uint64_t bits_to_write = xor_val >> trailing_zeros;
            writer_.write_bits(bits_to_write, meaningful_length);

            prev_leading_zeros_ = leading_zeros;
            prev_meaningful_length_ = meaningful_length;
        }
    }

    prev_val_ = val_bits;
}
