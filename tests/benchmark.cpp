#include "TSDB.h"
#include <iostream>
#include <vector>
#include <thread>
#include <chrono>
#include <atomic>
#include <random>
#include <algorithm>
#include <iomanip>
#include <mutex>

// Benchmark parameters
const int DURATION_SEC = 5;
const int WRITE_RATE_PER_SEC = 10000;
const int READ_RATE_PER_SEC = 1000;

const int NUM_WRITE_THREADS = 4;
const int NUM_READ_THREADS = 2;

const int WRITE_RATE_PER_THREAD = WRITE_RATE_PER_SEC / NUM_WRITE_THREADS;
const int READ_RATE_PER_THREAD = READ_RATE_PER_SEC / NUM_READ_THREADS;

std::atomic<bool> g_running(true);

struct ThreadStats {
    std::vector<uint64_t> latencies_us;
    uint64_t ops_completed = 0;
};

void run_writer(int thread_id, TSDB& db, ThreadStats& stats) {
    std::vector<std::string> metrics;
    for (int i = 0; i < 1000; ++i) {
        metrics.push_back("metric_" + std::to_string(i));
    }

    std::mt19937 rng(1337 + thread_id);
    std::uniform_int_distribution<size_t> metric_dist(0, metrics.size() - 1);
    std::uniform_real_distribution<double> val_dist(0.0, 100.0);

    auto target_delay = std::chrono::nanoseconds(1000000000ULL / WRITE_RATE_PER_THREAD);
    auto start_time = std::chrono::steady_clock::now();
    uint64_t ops = 0;

    stats.latencies_us.reserve(WRITE_RATE_PER_THREAD * DURATION_SEC * 2);

    while (g_running.load(std::memory_order_relaxed)) {
        auto op_start = std::chrono::steady_clock::now();

        std::string metric = metrics[metric_dist(rng)];
        int64_t ts = std::chrono::duration_cast<std::chrono::nanoseconds>(
            op_start.time_since_epoch()).count();
        double val = val_dist(rng);

        db.insert(metric, ts, val, {{"host", "server_" + std::to_string(thread_id)}});

        auto op_end = std::chrono::steady_clock::now();
        uint64_t latency = std::chrono::duration_cast<std::chrono::microseconds>(op_end - op_start).count();
        stats.latencies_us.push_back(latency);
        stats.ops_completed++;

        ops++;
        auto expected_elapsed = target_delay * ops;
        auto actual_elapsed = std::chrono::steady_clock::now() - start_time;
        if (actual_elapsed < expected_elapsed) {
            auto sleep_dur = expected_elapsed - actual_elapsed;
            if (sleep_dur > std::chrono::milliseconds(1)) {
                std::this_thread::sleep_for(sleep_dur - std::chrono::microseconds(500));
            }
            while (std::chrono::steady_clock::now() - start_time < expected_elapsed) {
                // busy spin
            }
        }
    }
}

void run_reader(int thread_id, TSDB& db, ThreadStats& stats) {
    std::vector<std::string> metrics;
    for (int i = 0; i < 1000; ++i) {
        metrics.push_back("metric_" + std::to_string(i));
    }

    std::mt19937 rng(2026 + thread_id);
    std::uniform_int_distribution<size_t> metric_dist(0, metrics.size() - 1);

    auto target_delay = std::chrono::nanoseconds(1000000000ULL / READ_RATE_PER_THREAD);
    auto start_time = std::chrono::steady_clock::now();
    uint64_t ops = 0;

    stats.latencies_us.reserve(READ_RATE_PER_THREAD * DURATION_SEC * 2);

    while (g_running.load(std::memory_order_relaxed)) {
        auto op_start = std::chrono::steady_clock::now();

        std::string metric = metrics[metric_dist(rng)];
        // Perform a read query
        auto results = db.query(metric);

        auto op_end = std::chrono::steady_clock::now();
        uint64_t latency = std::chrono::duration_cast<std::chrono::microseconds>(op_end - op_start).count();
        stats.latencies_us.push_back(latency);
        stats.ops_completed++;

        ops++;
        auto expected_elapsed = target_delay * ops;
        auto actual_elapsed = std::chrono::steady_clock::now() - start_time;
        if (actual_elapsed < expected_elapsed) {
            auto sleep_dur = expected_elapsed - actual_elapsed;
            if (sleep_dur > std::chrono::milliseconds(1)) {
                std::this_thread::sleep_for(sleep_dur - std::chrono::microseconds(500));
            }
            while (std::chrono::steady_clock::now() - start_time < expected_elapsed) {
                // busy spin
            }
        }
    }
}

void print_percentiles(const std::string& name, std::vector<uint64_t>& latencies, double elapsed_sec, uint64_t total_ops) {
    if (latencies.empty()) {
        std::cout << name << " - No operations completed.\n";
        return;
    }

    std::sort(latencies.begin(), latencies.end());
    
    double p50 = latencies[static_cast<size_t>(latencies.size() * 0.50)];
    double p90 = latencies[static_cast<size_t>(latencies.size() * 0.90)];
    double p95 = latencies[static_cast<size_t>(latencies.size() * 0.95)];
    double p99 = latencies[static_cast<size_t>(latencies.size() * 0.99)];

    double actual_rate = total_ops / elapsed_sec;

    std::cout << std::left << std::setw(15) << name 
              << " | Total Ops: " << std::setw(8) << total_ops
              << " | Actual Rate: " << std::setw(10) << std::fixed << std::setprecision(2) << actual_rate << " ops/sec\n"
              << "Latencies (us) | p50: " << std::setw(6) << p50
              << " | p90: " << std::setw(6) << p90
              << " | p95: " << std::setw(6) << p95
              << " | p99: " << std::setw(6) << p99 << "\n\n";
}

int main() {
    std::cout << "Starting Time-Series Database Performance Benchmark...\n";
    std::cout << "Target Write Rate: " << WRITE_RATE_PER_SEC << " ops/sec (" << NUM_WRITE_THREADS << " threads)\n";
    std::cout << "Target Read Rate:  " << READ_RATE_PER_SEC << " ops/sec (" << NUM_READ_THREADS << " threads)\n";
    std::cout << "Duration:          " << DURATION_SEC << " seconds\n\n";

    TSDB db;

    // Warm-up database with some initial values to make sure there's data to read
    for (int i = 0; i < 10000; ++i) {
        db.insert("metric_" + std::to_string(i % 1000), 1600000000 + i * 60, 25.5 + i, {{"host", "warmup"}});
    }

    std::vector<ThreadStats> write_stats(NUM_WRITE_THREADS);
    std::vector<ThreadStats> read_stats(NUM_READ_THREADS);

    std::vector<std::thread> threads;
    auto benchmark_start = std::chrono::steady_clock::now();

    for (int i = 0; i < NUM_WRITE_THREADS; ++i) {
        threads.emplace_back(run_writer, i, std::ref(db), std::ref(write_stats[i]));
    }
    for (int i = 0; i < NUM_READ_THREADS; ++i) {
        threads.emplace_back(run_reader, i, std::ref(db), std::ref(read_stats[i]));
    }

    std::this_thread::sleep_for(std::chrono::seconds(DURATION_SEC));
    g_running.store(false, std::memory_order_relaxed);

    for (auto& t : threads) {
        t.join();
    }

    auto benchmark_end = std::chrono::steady_clock::now();
    double elapsed_sec = std::chrono::duration_cast<std::chrono::milliseconds>(benchmark_end - benchmark_start).count() / 1000.0;

    // Combine stats
    std::vector<uint64_t> all_write_latencies;
    uint64_t total_writes = 0;
    for (const auto& s : write_stats) {
        all_write_latencies.insert(all_write_latencies.end(), s.latencies_us.begin(), s.latencies_us.end());
        total_writes += s.ops_completed;
    }

    std::vector<uint64_t> all_read_latencies;
    uint64_t total_reads = 0;
    for (const auto& s : read_stats) {
        all_read_latencies.insert(all_read_latencies.end(), s.latencies_us.begin(), s.latencies_us.end());
        total_reads += s.ops_completed;
    }

    std::cout << "--- Benchmark Results ---\n";
    print_percentiles("Writes", all_write_latencies, elapsed_sec, total_writes);
    print_percentiles("Reads", all_read_latencies, elapsed_sec, total_reads);

    return 0;
}
