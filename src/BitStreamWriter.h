#pragma once
#include <vector>
#include <cstdint>

class BitStreamWriter {
public:
    BitStreamWriter() : bit_offset_(0) {}

    // Writes `num_bits` from `value` into the stream.
    // The bits are written from most significant (in the `num_bits` window) to least.
    void write_bits(uint64_t value, size_t num_bits);

    // Writes a single bit
    void write_bit(bool bit);

    const std::vector<uint8_t>& get_buffer() const { return buffer_; }

private:
    std::vector<uint8_t> buffer_;
    uint8_t bit_offset_; // 0 to 7 indicating which bit in the current byte we are at
};
