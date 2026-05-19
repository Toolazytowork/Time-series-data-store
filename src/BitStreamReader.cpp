#include "BitStreamReader.h"
#include <stdexcept>

bool BitStreamReader::read_bit() {
    if (is_eof()) {
        throw std::out_of_range("BitStreamReader: End of stream");
    }
    bool bit = (buffer_[byte_idx_] >> (7 - bit_offset_)) & 1;
    bit_offset_++;
    if (bit_offset_ == 8) {
        bit_offset_ = 0;
        byte_idx_++;
    }
    return bit;
}

uint64_t BitStreamReader::read_bits(size_t num_bits) {
    if (num_bits == 0) return 0;
    
    uint64_t value = 0;
    for (size_t i = 0; i < num_bits; ++i) {
        value = (value << 1) | (read_bit() ? 1 : 0);
    }
    return value;
}
