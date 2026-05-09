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
