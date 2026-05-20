#include "GorillaTimestamp.h"

template<typename T>
static T sign_extend(uint64_t val, size_t bits) {
    if (val & (1ULL << (bits - 1))) {
        return static_cast<T>(val | ~((1ULL << bits) - 1));
    }
    return static_cast<T>(val);
}

void GorillaTimestampCompressor::append(int64_t timestamp) {
    if (is_first_) {
        writer_.write_bits(timestamp, 64);
        prev_timestamp_ = timestamp;
        is_first_ = false;
        return;
    }

    int64_t delta = timestamp - prev_timestamp_;
    int64_t dod = delta - prev_delta_;

    if (dod == 0) {
        writer_.write_bit(false); // 0
    } else if (dod >= -63 && dod <= 64) {
        writer_.write_bits(0b10, 2);
        writer_.write_bits(dod & 0x7F, 7);
    } else if (dod >= -255 && dod <= 256) {
        writer_.write_bits(0b110, 3);
        writer_.write_bits(dod & 0x1FF, 9);
    } else if (dod >= -2047 && dod <= 2048) {
        writer_.write_bits(0b1110, 4);
        writer_.write_bits(dod & 0xFFF, 12);
    } else if (dod >= -2147483647LL && dod <= 2147483648LL) {
        writer_.write_bits(0b11110, 5);
        writer_.write_bits(dod & 0xFFFFFFFF, 32);
    } else {
        writer_.write_bits(0b11111, 5);
        writer_.write_bits(dod, 64);
    }

    prev_delta_ = delta;
    prev_timestamp_ = timestamp;
}

int64_t GorillaTimestampDecompressor::read() {
    if (is_first_) {
        prev_timestamp_ = reader_.read_bits(64);
        is_first_ = false;
        return prev_timestamp_;
    }

    int64_t dod = 0;
    if (reader_.read_bit() == 0) {
        dod = 0;
    } else if (reader_.read_bit() == 0) { // prefix 10
        dod = sign_extend<int64_t>(reader_.read_bits(7), 7);
    } else if (reader_.read_bit() == 0) { // prefix 110
        dod = sign_extend<int64_t>(reader_.read_bits(9), 9);
    } else if (reader_.read_bit() == 0) { // prefix 1110
        dod = sign_extend<int64_t>(reader_.read_bits(12), 12);
    } else if (reader_.read_bit() == 0) { // prefix 11110
        dod = sign_extend<int64_t>(reader_.read_bits(32), 32);
    } else {                              // prefix 11111
        dod = reader_.read_bits(64);
    }

    prev_delta_ += dod;
    prev_timestamp_ += prev_delta_;
    return prev_timestamp_;
}
