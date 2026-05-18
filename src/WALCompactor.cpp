#include "WALCompactor.h"
#include "WALWriter.h"
#include <cstdio>

size_t WALCompactor::compact(const TSDB& db, const std::string& target_wal_path) {
    std::string tmp_path = target_wal_path + ".tmp";
    
    // Remove tmp file if it exists to ensure clean state
    std::remove(tmp_path.c_str());

    size_t count = 0;
    {
        // Open a fresh WALWriter for the tmp file. We preallocate here as well.
        WALWriter writer(tmp_path);
        
        db.dump_all([&](const std::string& metric, const DataPoint& dp) {
            if (writer.append(metric, dp)) {
                count++;
            }
        });
        // Destructor of WALWriter will ftruncate and close the file safely, cutting off trailing zeros.
    }

    // Atomically replace the old WAL file with the compacted one
    std::rename(tmp_path.c_str(), target_wal_path.c_str());

    return count;
}
