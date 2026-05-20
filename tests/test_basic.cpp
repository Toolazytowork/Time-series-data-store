#include <gtest/gtest.h>
#include "DataPoint.h"
#include "TimeSeries.h"
#include "TSDB.h"

TEST(DataPointTest, SizeAndAlignment) {
    EXPECT_EQ(sizeof(DataPoint), 16);
    EXPECT_EQ(alignof(DataPoint), 16);
}

TEST(TimeSeriesTest, BasicInstantiation) {
    TimeSeries ts;
    ts.metric_name = "cpu.usage";
    EXPECT_EQ(ts.data.size(), 0);
}

TEST(TSDBTest, BasicInstantiation) {
    TSDB db;
    // Just testing that the DB can be instantiated without errors.
    EXPECT_TRUE(true);
}

TEST(TSDBTest, InsertionAndQuery) {
    TSDB db;
    db.insert("cpu.usage", 1620000000000, 45.2);
    db.insert("cpu.usage", 1620000001000, 46.1);
    
    auto result = db.query("cpu.usage");
    ASSERT_EQ(result.size(), 2);
    EXPECT_EQ(result[0].timestamp, 1620000000000);
    EXPECT_DOUBLE_EQ(result[0].value, 45.2);
    EXPECT_EQ(result[1].timestamp, 1620000001000);
    EXPECT_DOUBLE_EQ(result[1].value, 46.1);

    auto empty_result = db.query("mem.usage");
    EXPECT_TRUE(empty_result.empty());
}

#include <thread>

TEST(TSDBTest, ConcurrentInsertAndQuery) {
    TSDB db;
    const int num_threads = 10;
    const int num_inserts = 1000;
    std::vector<std::thread> threads;

    for (int i = 0; i < num_threads; ++i) {
        threads.emplace_back([&db, i, num_inserts]() {
            for (int j = 0; j < num_inserts; ++j) {
                db.insert("metric_" + std::to_string(i), j, j * 1.5);
            }
        });
    }

    for (auto& t : threads) {
        t.join();
    }

    for (int i = 0; i < num_threads; ++i) {
        auto result = db.query("metric_" + std::to_string(i));
        EXPECT_EQ(result.size(), num_inserts);
    }
}

TEST(TSDBTest, TagIndexing) {
    TSDB db;
    db.insert("cpu.usage.host1", 1000, 45.0, {{"host", "server1"}, {"region", "us-east"}});
    db.insert("cpu.usage.host2", 1000, 50.0, {{"host", "server2"}, {"region", "us-east"}});
    db.insert("mem.usage.host1", 1000, 80.0, {{"host", "server1"}, {"region", "us-west"}});

    const auto& index = db.get_tag_index();
    
    auto us_east_metrics = index.get_metrics_by_tag("region", "us-east");
    EXPECT_EQ(us_east_metrics.size(), 2);
    EXPECT_TRUE(us_east_metrics.count("cpu.usage.host1"));
    EXPECT_TRUE(us_east_metrics.count("cpu.usage.host2"));

    auto server1_metrics = index.get_metrics_by_tag("host", "server1");
    EXPECT_EQ(server1_metrics.size(), 2);
    EXPECT_TRUE(server1_metrics.count("cpu.usage.host1"));
    EXPECT_TRUE(server1_metrics.count("mem.usage.host1"));

    auto unknown_metrics = index.get_metrics_by_tag("host", "server3");
    EXPECT_TRUE(unknown_metrics.empty());
}

TEST(TSDBTest, ComplexQuery) {
    TSDB db;
    // Insert some points
    db.insert("cpu.usage.host1", 100, 45.0, {{"host", "server1"}, {"region", "us-east"}});
    db.insert("cpu.usage.host1", 200, 46.0);
    db.insert("cpu.usage.host1", 300, 47.0);

    db.insert("cpu.usage.host2", 150, 50.0, {{"host", "server2"}, {"region", "us-east"}});
    db.insert("cpu.usage.host2", 250, 51.0);

    db.insert("mem.usage.host1", 100, 80.0, {{"host", "server1"}, {"region", "us-east"}});
    db.insert("mem.usage.host1", 200, 81.0);

    // Query: host=server1 AND region=us-east, time [150, 250]
    std::unordered_map<std::string, std::string> tags = {{"host", "server1"}, {"region", "us-east"}};
    auto results = db.query_complex(tags, 150, 250);

    // Expecting cpu.usage.host1 and mem.usage.host1
    EXPECT_EQ(results.size(), 2);
    
    ASSERT_TRUE(results.count("cpu.usage.host1"));
    EXPECT_EQ(results["cpu.usage.host1"].size(), 1); // Only timestamp 200
    EXPECT_EQ(results["cpu.usage.host1"][0].timestamp, 200);

    ASSERT_TRUE(results.count("mem.usage.host1"));
    EXPECT_EQ(results["mem.usage.host1"].size(), 1); // Only timestamp 200
    EXPECT_EQ(results["mem.usage.host1"][0].timestamp, 200);

    // Query: host=server2, time [100, 300]
    auto results2 = db.query_complex({{"host", "server2"}}, 100, 300);
    EXPECT_EQ(results2.size(), 1);
    ASSERT_TRUE(results2.count("cpu.usage.host2"));
    EXPECT_EQ(results2["cpu.usage.host2"].size(), 2);
}

#include "WALWriter.h"
#include <fstream>
#include <cstdio>

TEST(WALTest, AppendAndVerifySize) {
    std::string filename = "test_wal.log";
    std::remove(filename.c_str());

    {
        WALWriter wal(filename);
        DataPoint dp1{1000, 45.5};
        EXPECT_TRUE(wal.append("cpu", dp1));

        DataPoint dp2{2000, 50.0};
        EXPECT_TRUE(wal.append("mem", dp2));
    }

    // Verify file size
    // cpu log: 2 + 3 ("cpu") + 8 + 8 = 21 bytes
    // mem log: 2 + 3 ("mem") + 8 + 8 = 21 bytes
    // total: 42 bytes
    std::ifstream in(filename, std::ios::binary | std::ios::ate);
    EXPECT_EQ(in.tellg(), 42);
    
    std::remove(filename.c_str());
}

#include "WALReader.h"

TEST(WALTest, CrashRecovery) {
    std::string filename = "test_crash_recovery.log";
    std::remove(filename.c_str());

    {
        // Simulate a running TSDB that writes to WAL
        WALWriter wal(filename); // allocates 64MB memory map
        DataPoint dp1{100, 1.1};
        wal.append("metric.a", dp1);
        DataPoint dp2{200, 2.2};
        wal.append("metric.b", dp2);
        
        // We simulate a hard crash by reading the file while it is still open 
        // and full of trailing zeros, before the WALWriter destructor truncates it.
        TSDB db_recovered;
        size_t recovered = WALReader::recover(filename, db_recovered);
        
        EXPECT_EQ(recovered, 2);
        
        auto res_a = db_recovered.query("metric.a");
        ASSERT_EQ(res_a.size(), 1);
        EXPECT_EQ(res_a[0].timestamp, 100);
        EXPECT_EQ(res_a[0].value, 1.1);

        auto res_b = db_recovered.query("metric.b");
        ASSERT_EQ(res_b.size(), 1);
    } // WALWriter destructor cleanly truncates file length here

    std::remove(filename.c_str());
}

#include "WALManager.h"

TEST(WALTest, Compaction) {
    std::string filename = "test_compaction.log";
    std::remove(filename.c_str());

    {
        TSDB db;
        // Threshold = 2
        WALManager manager(db, filename, 2);
        
        manager.insert("metric.x", 10, 1.0);
        manager.insert("metric.x", 20, 2.0); // Threshold reached, compacts here!
        manager.insert("metric.y", 30, 3.0);
    }
    
    // Validate by reading the compacted WAL
    TSDB db_recovered;
    size_t recovered = WALReader::recover(filename, db_recovered);
    
    // We expect 3 points recovered because the compaction kept all active memory state
    EXPECT_EQ(recovered, 3);
    
    auto res_x = db_recovered.query("metric.x");
    EXPECT_EQ(res_x.size(), 2);
    
    auto res_y = db_recovered.query("metric.y");
    EXPECT_EQ(res_y.size(), 1);

    std::remove(filename.c_str());
}

#include "BitStreamWriter.h"
#include "BitStreamReader.h"

TEST(BitStreamTest, WriteAndRead) {
    BitStreamWriter writer;
    writer.write_bit(true); // 1
    writer.write_bit(false); // 0
    writer.write_bits(0b1011, 4); // 1011
    writer.write_bits(0xDEADBEEF, 32);

    const auto& buffer = writer.get_buffer();
    
    BitStreamReader reader(buffer);
    EXPECT_TRUE(reader.read_bit());
    EXPECT_FALSE(reader.read_bit());
    EXPECT_EQ(reader.read_bits(4), 0b1011);
    EXPECT_EQ(reader.read_bits(32), 0xDEADBEEF);
}

#include "GorillaTimestamp.h"

TEST(GorillaTest, TimestampCompression) {
    BitStreamWriter writer;
    GorillaTimestampCompressor compressor(writer);

    std::vector<int64_t> timestamps = {
        1600000000, 
        1600000060, // +60 (dod = 60)
        1600000120, // +60 (dod = 0)
        1600000179, // +59 (dod = -1)
        1600000300, // +121 (dod = 62)
        1600001000, // +700 (dod = 579)
        1600005000  // +4000 (dod = 3300)
    };

    for (int64_t ts : timestamps) {
        compressor.append(ts);
    }

    BitStreamReader reader(writer.get_buffer());
    GorillaTimestampDecompressor decompressor(reader);

    for (int64_t ts : timestamps) {
        EXPECT_EQ(decompressor.read(), ts);
    }
}
