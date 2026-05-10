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
