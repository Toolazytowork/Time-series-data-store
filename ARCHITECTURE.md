# In-Memory Time Series Database (TSDB) Architecture

## Role & Mission
You are a Staff-Level Systems Engineer specializing in low-level C++ infrastructure and database internals. Your mission is to implement a High-Performance, Concurrent, In-Memory Time Series Database (TSDB). 

## Technical Constraints
*   **Language:** Modern C++ (C++17 or C++20).
*   **Build System:** CMake.
*   **Testing:** Google Test (GTest) for all unit tests.
*   **Environment:** Linux (POSIX compliant).
*   **Performance Goals:** Handle 10,000+ writes/sec and 1,000+ complex reads/sec with a p99 latency of < 2ms.
*   **Dependencies:** Zero external dependencies except GTest. Rely on the standard library and POSIX system calls (`mmap`, `pthreads`).
*   **Scale:** Up to 100,000 metrics, 24-hour retention.

## Core Data Model
*   **DataPoint:** Cache-aligned struct containing a 64-bit UNIX timestamp (milliseconds) and a 64-bit double value. Must avoid padding.
*   Example: `(1620000000000, "cpu.usage", 45.2, {"host": "server1", "datacenter": "us-west"})`

## System Components

### 1. The Storage Engine (Mechanical Sympathy)
*   Map `metric_name` (string) to a contiguous `std::vector<DataPoint>` to maximize CPU cache hits.
*   **Concurrency:** Strict prohibition on global mutexes. Implement lock-striping (sharding) for the in-memory store. Use `std::shared_mutex` to allow concurrent readers while blocking writers safely per shard.

### 2. Inverted Index
*   Map `tag_key:tag_value` to a list of metric names (e.g., `std::unordered_set<std::string>`).
*   Querying must support fast tag-based filtering (including compound AND logic) using set intersection on this index to strictly avoid full table scans.

### 3. Durability & Crash Recovery (WAL)
*   Implement a Write-Ahead Log (WAL) for durability. Writes must append to the WAL before memory ingestion completes.
*   **Optimization:** Use memory-mapped files (`mmap`) for the WAL to achieve zero-copy disk writes, relying on the Linux kernel for page flushing.
*   Implement a crash recovery sequence on instantiation to parse the WAL and rebuild memory state.

### 4. Data Compression
*   Implement compression based on the Facebook Gorilla TSDB paper.
*   Use Delta-of-Delta compression for timestamps.
*   Use XOR-based compression for 64-bit floating-point values.