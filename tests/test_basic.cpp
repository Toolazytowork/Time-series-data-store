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
