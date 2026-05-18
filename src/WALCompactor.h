#pragma once
#include <string>
#include "TSDB.h"

class WALCompactor {
public:
    // Compacts the current TSDB state into a new WAL file, then atomically replaces the old WAL.
    // Returns the number of points written to the new WAL.
    static size_t compact(const TSDB& db, const std::string& target_wal_path);
};
