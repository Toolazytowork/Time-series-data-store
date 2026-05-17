#pragma once
#include <string>
#include "TSDB.h"

class WALReader {
public:
    // Reads a WAL file and replays all data points into the provided TSDB instance.
    // Returns the number of points successfully recovered.
    static size_t recover(const std::string& file_path, TSDB& db);
};
