#include "WALWriter.h"
#include <fcntl.h>
#include <unistd.h>
#include <sys/mman.h>
#include <sys/stat.h>
#include <stdexcept>
#include <cstring>

WALWriter::WALWriter(const std::string& file_path, size_t file_size)
    : file_size_(file_size) {
    
    // Open the file
    fd_ = open(file_path.c_str(), O_RDWR | O_CREAT, 0644);
    if (fd_ < 0) {
        throw std::runtime_error("Failed to open WAL file for mmap: " + file_path);
    }

    struct stat st;
    if (fstat(fd_, &st) == 0 && st.st_size > 0) {
        offset_.store(st.st_size);
    } else {
        offset_.store(0);
    }

    // Set file size to reserve space for the memory mapped region
    if (ftruncate(fd_, file_size_) != 0) {
        close(fd_);
        throw std::runtime_error("Failed to ftruncate WAL file.");
    }

    // Map the region
    mapped_region_ = static_cast<char*>(mmap(nullptr, file_size_, PROT_READ | PROT_WRITE, MAP_SHARED, fd_, 0));
    if (mapped_region_ == MAP_FAILED) {
        close(fd_);
        throw std::runtime_error("Failed to mmap WAL file.");
    }
}

WALWriter::~WALWriter() {
    if (mapped_region_ != MAP_FAILED) {
        msync(mapped_region_, file_size_, MS_SYNC);
        munmap(mapped_region_, file_size_);
    }
    if (fd_ >= 0) {
        // Truncate to the actual bytes written to remove unused trailing zeros
        ftruncate(fd_, offset_.load());
        close(fd_);
    }
}

bool WALWriter::append(const std::string& metric_name, const DataPoint& dp) {
    uint16_t name_len = static_cast<uint16_t>(metric_name.size());
    size_t total_size = sizeof(uint16_t) + name_len + sizeof(int64_t) + sizeof(double);

    // Lock-free atomic offset allocation
    size_t current_offset = offset_.fetch_add(total_size, std::memory_order_relaxed);

    if (current_offset + total_size > file_size_) {
        // Exceeded mapped region, basic implementation fails out
        return false; 
    }

    char* dest = mapped_region_ + current_offset;

    std::memcpy(dest, &name_len, sizeof(uint16_t));
    dest += sizeof(uint16_t);

    std::memcpy(dest, metric_name.data(), name_len);
    dest += name_len;

    std::memcpy(dest, &dp.timestamp, sizeof(int64_t));
    dest += sizeof(int64_t);

    std::memcpy(dest, &dp.value, sizeof(double));

    return true;
}
