#pragma once
#include <vector>
#include <cstdint>

class BitStreamReader {
public:
    explicit BitStreamReader(const std::vector<uint8_t>& buffer)
        : buffer_(buffer), byte_idx_(0), bit_offset_(0) {}

    // Reads `num_bits` from the stream. Returns the value.
    uint64_t read_bits(size_t num_bits);

    // Reads a single bit
    bool read_bit();

    bool is_eof() const {
        return byte_idx_ >= buffer_.size();
    }

private:
    const std::vector<uint8_t>& buffer_;
    size_t byte_idx_;
    uint8_t bit_offset_;
};
