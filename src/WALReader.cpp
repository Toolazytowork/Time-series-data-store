#include "WALReader.h"
#include <fstream>
#include <vector>

size_t WALReader::recover(const std::string& file_path, TSDB& db) {
    std::ifstream in(file_path, std::ios::binary);
    if (!in) {
        return 0; // File might not exist yet, which is fine on fresh startup.
    }

    size_t count = 0;
    while (in.peek() != EOF) {
        uint16_t name_len = 0;
        if (!in.read(reinterpret_cast<char*>(&name_len), sizeof(name_len))) break;
        if (name_len == 0) break; // Reached trailing zero region from incomplete mmap ftruncate

        std::string metric_name(name_len, '\0');
        if (!in.read(&metric_name[0], name_len)) break;

        int64_t timestamp = 0;
        if (!in.read(reinterpret_cast<char*>(&timestamp), sizeof(timestamp))) break;

        double value = 0.0;
        if (!in.read(reinterpret_cast<char*>(&value), sizeof(value))) break;

        db.insert(metric_name, timestamp, value);
        count++;
    }

    return count;
}
