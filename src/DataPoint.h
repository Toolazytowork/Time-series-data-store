#pragma once
#include <cstdint>

// Cache-aligned DataPoint. Avoids padding.
// sizeof(DataPoint) == 16 bytes.
struct alignas(16) DataPoint {
    int64_t timestamp; // 64-bit UNIX timestamp in milliseconds
    double value;      // 64-bit double value
};
