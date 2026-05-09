# TSDB 15-Day Implementation Roadmap

## Phase 1: Skeleton & Build System
- [x] Day 1: Output the `CMakeLists.txt`, configure GTest, and create core headers (`DataPoint.h`, `TimeSeries.h`, `TSDB.h`). Outline data structures and alignment.
- [ ] Day 2: Implement basic memory layout (cache-aligned DataPoint) and the un-sharded `TSDB` class mapping metric names to vectors. Write basic tests.

## Phase 2: Core Storage, Concurrency & Indexing
- [ ] Day 3: Implement the sharded memory store (e.g., 16 buckets) in `TSDB.cpp` using a hashing mechanism. Update basic insertion tests.
- [ ] Day 4: Implement `std::shared_mutex` lock-striping for concurrent access. Write multi-threaded tests using `std::thread` to prove absence of data races.
- [ ] Day 5: Implement `TagIndex` class (Inverted Index) for tag ingestion. Update insertion logic to populate the index. Write indexing unit tests.
- [ ] Day 6: Implement the complex `query` function (time range + set intersection for AND logic on tags). Write comprehensive query tests.

## Phase 3: Durability & Crash Recovery (WAL)
- [ ] Day 7: Implement standard append-only `WALWriter` with POSIX I/O (`O_APPEND | O_DSYNC`). Serialize DataPoints to binary. Write disk I/O tests.
- [ ] Day 8: Upgrade `WALWriter` to use zero-copy `mmap` with large memory-mapped regions and atomic offset pointers. Update tests.
- [ ] Day 9: Implement `WALReader` for crash recovery on startup. Write tests simulating TSDB destruction and successful state rebuild.
- [ ] Day 10: Implement basic WAL compaction strategy (e.g., threshold-based snapshotting and WAL clearing).

## Phase 4: Data Compression (Gorilla Algorithm)
- [ ] Day 11: Implement a `BitStream` writer/reader class.
- [ ] Day 12: Implement Delta-of-Delta timestamp compression using the BitStream.
- [ ] Day 13: Implement XOR-based 64-bit floating-point value compression. Integrate compression into the main engine.

## Phase 5: Benchmarking & Finalization
- [ ] Day 14: Generate `benchmark.cpp` using multiple threads to simulate 10,000 writes/sec and 1,000 reads/sec. Measure p99 latency.
- [ ] Day 15: Final code cleanup, ensure strict adherence to zero-padding/cache-alignment, and write the project README.