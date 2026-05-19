#include "BitStreamWriter.h"

void BitStreamWriter::write_bit(bool bit) {
    if (bit_offset_ == 0) {
        buffer_.push_back(0);
    }
    if (bit) {
        buffer_.back() |= (1 << (7 - bit_offset_));
    }
    bit_offset_ = (bit_offset_ + 1) % 8;
}

void BitStreamWriter::write_bits(uint64_t value, size_t num_bits) {
    if (num_bits == 0) return;
    
    // Write bits one by one from most significant to least
    for (int i = static_cast<int>(num_bits) - 1; i >= 0; --i) {
        bool b = (value >> i) & 1;
        write_bit(b);
    }
}
