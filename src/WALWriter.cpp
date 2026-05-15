#include "WALWriter.h"
#include <fcntl.h>
#include <unistd.h>
#include <stdexcept>
#include <cstring>
#include <vector>

WALWriter::WALWriter(const std::string& file_path) {
    // Open with O_WRONLY | O_CREAT | O_APPEND | O_DSYNC
    fd_ = open(file_path.c_str(), O_WRONLY | O_CREAT | O_APPEND | O_DSYNC, 0644);
    if (fd_ < 0) {
        throw std::runtime_error("Failed to open WAL file: " + file_path);
    }
}

WALWriter::~WALWriter() {
    if (fd_ >= 0) {
        close(fd_);
    }
}

bool WALWriter::append(const std::string& metric_name, const DataPoint& dp) {
    std::lock_guard<std::mutex> lock(mutex_);

    // Binary format:
    // [uint16_t name_len] [char[] name] [int64_t timestamp] [double value]
    uint16_t name_len = static_cast<uint16_t>(metric_name.size());
    
    // Calculate total buffer size
    size_t total_size = sizeof(uint16_t) + name_len + sizeof(int64_t) + sizeof(double);
    std::vector<char> buf(total_size);
    
    size_t offset = 0;
    std::memcpy(buf.data() + offset, &name_len, sizeof(uint16_t));
    offset += sizeof(uint16_t);
    
    std::memcpy(buf.data() + offset, metric_name.data(), name_len);
    offset += name_len;
    
    std::memcpy(buf.data() + offset, &dp.timestamp, sizeof(int64_t));
    offset += sizeof(int64_t);
    
    std::memcpy(buf.data() + offset, &dp.value, sizeof(double));
    offset += sizeof(double);
    
    ssize_t written = write(fd_, buf.data(), total_size);
    
    return written == static_cast<ssize_t>(total_size);
}
