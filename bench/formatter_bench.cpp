//
// formatter_bench.cpp — benchmark the simple_formatter in isolation
//

#include "benchmark/benchmark.h"
#include "spdlite/common.h"

#include "spdlite/formatter.h"

using namespace spdlite;

// Format with short payload
static void bench_format_short(benchmark::State &state) {
    simple_formatter fmt;
    memory_buf_t buf;
    log_msg msg("mylogger", level::info, "Hello world");
    for (auto _ : state) {
        buf.clear();
        fmt.format(msg, buf);
        benchmark::DoNotOptimize(buf.data());
    }
}

// Format with typical payload
static void bench_format_typical(benchmark::State &state) {
    simple_formatter fmt;
    memory_buf_t buf;
    log_msg msg("mylogger", level::info, "Hello logger: msg number 12345...............");
    for (auto _ : state) {
        buf.clear();
        fmt.format(msg, buf);
        benchmark::DoNotOptimize(buf.data());
    }
}

// Format with long payload (500 bytes)
static void bench_format_long(benchmark::State &state) {
    simple_formatter fmt;
    memory_buf_t buf;
    log_msg msg("mylogger", level::info,
        "Lorem ipsum dolor sit amet, consectetur adipiscing elit. Vestibulum pharetra metus cursus "
        "lacus placerat congue. Nulla egestas, mauris a tincidunt tempus, enim lectus volutpat mi, "
        "eu consequat sem libero nec massa. In dapibus ipsum a diam rhoncus gravida. Etiam non dapibus eros. Donec "
        "fringilla dui sed augue pretium, nec scelerisque est maximus. Nullam convallis, sem nec blandit maximus, "
        "nisi turpis ornare nisl, sit amet volutpat neque massa eu odio. Maecenas malesuada quam ex.");
    for (auto _ : state) {
        buf.clear();
        fmt.format(msg, buf);
        benchmark::DoNotOptimize(buf.data());
    }
}

// Just the timestamp (cached path — same second)
static void bench_timestamp_cached(benchmark::State &state) {
    simple_formatter fmt;
    memory_buf_t buf;
    log_msg msg("x", level::info, "x");
    // Warm the cache
    fmt.format(msg, buf);
    for (auto _ : state) {
        buf.clear();
        fmt.format(msg, buf);
        benchmark::DoNotOptimize(buf.data());
    }
}

// Timestamp with varying milliseconds (simulates real traffic)
static void bench_timestamp_varying_ms(benchmark::State &state) {
    simple_formatter fmt;
    memory_buf_t buf;
    int ms = 0;
    auto base = std::chrono::system_clock::now();
    for (auto _ : state) {
        auto t = base + std::chrono::milliseconds(ms++ % 1000);
        log_msg msg(t, "x", level::info, "x");
        buf.clear();
        fmt.format(msg, buf);
        benchmark::DoNotOptimize(buf.data());
    }
}

BENCHMARK(bench_format_short);
BENCHMARK(bench_format_typical);
BENCHMARK(bench_format_long);
BENCHMARK(bench_timestamp_cached);
BENCHMARK(bench_timestamp_varying_ms);

BENCHMARK_MAIN();
