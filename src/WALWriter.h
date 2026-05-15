#pragma once
#include <string>
#include <mutex>
#include "DataPoint.h"

class WALWriter {
public:
    explicit WALWriter(const std::string& file_path);
    ~WALWriter();

    // Appends a DataPoint to the WAL. Returns true on success.
    bool append(const std::string& metric_name, const DataPoint& dp);

private:
    int fd_;
    std::mutex mutex_;
};
