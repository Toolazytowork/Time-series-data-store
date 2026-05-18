#pragma once
#include <string>
#include <memory>
#include "TSDB.h"
#include "WALWriter.h"
#include "WALCompactor.h"
#include "WALReader.h"

class WALManager {
public:
    // Initializes the database from an existing WAL, then compacts the WAL.
    // Future inserts are monitored; when writes exceed the threshold, a new compaction is triggered.
    WALManager(TSDB& db, const std::string& wal_path, size_t compaction_threshold = 10000)
        : db_(db), wal_path_(wal_path), threshold_(compaction_threshold), writes_(0) {
        
        // 1. Recover existing WAL
        WALReader::recover(wal_path_, db_);
        
        // 2. Always compact on startup. This safely resets the WAL payload and 
        // guarantees our mmap WALWriter can start appending cleanly from offset 0.
        WALCompactor::compact(db_, wal_path_);
        
        // 3. Open new WALWriter
        writer_ = std::make_unique<WALWriter>(wal_path_);
    }

    bool insert(const std::string& metric_name, int64_t timestamp, double value, 
                const std::unordered_map<std::string, std::string>& tags = {}) {
        
        DataPoint dp{timestamp, value};
        
        // 1. Write to WAL first for durability
        if (!writer_->append(metric_name, dp)) {
            return false;
        }

        // 2. Write to memory
        db_.insert(metric_name, timestamp, value, tags);

        // 3. Check compaction threshold
        if (++writes_ >= threshold_) {
            compact();
        }

        return true;
    }

    // Force a manual compaction
    void compact() {
        // Destroy current writer so we can safely replace the file
        writer_.reset();
        
        WALCompactor::compact(db_, wal_path_);
        
        // Re-open writer
        writer_ = std::make_unique<WALWriter>(wal_path_);
        writes_ = 0;
    }

private:
    TSDB& db_;
    std::string wal_path_;
    size_t threshold_;
    size_t writes_;
    std::unique_ptr<WALWriter> writer_;
};
