#pragma once
#include <string>
#include <atomic>
#include <mutex>
#include "DataPoint.h"

class WALWriter {
public:
    // Uses a large initial memory-mapped region (default 64MB)
    explicit WALWriter(const std::string& file_path, size_t file_size = 1024 * 1024 * 64);
    ~WALWriter();

    // Appends a DataPoint to the WAL using zero-copy mmap. Returns true on success.
    bool append(const std::string& metric_name, const DataPoint& dp);

private:
    int fd_;
    char* mapped_region_;
    size_t file_size_;
    std::atomic<size_t> offset_;
};
