//
// formatter_bench.cpp - benchmark the simple_formatter in isolation
//

#include "benchmark/benchmark.h"
#include "spdlite/formatter.h"

using namespace spdlite;

// appends: header + payload + \r?\n - mirrors what logger does per call
static inline void format_line(simple_formatter &fmt, memory_buf_t &dest,
                               log_clock::time_point t, level lvl, string_view_t payload) {
    fmt.format_header(t, lvl, dest);
    dest.append(payload.data(), payload.data() + payload.size());
#ifdef _WIN32
    dest.push_back('\r');
#endif
    dest.push_back('\n');
}

// Format with short payload
static void bench_format_short(benchmark::State &state) {
    simple_formatter fmt("mylogger");
    memory_buf_t buf;
    auto now = log_clock::now();
    for (auto _ : state) {
        buf.clear();
        format_line(fmt, buf, now, level::info, "Hello world");
        benchmark::DoNotOptimize(buf.data());
    }
}

// Format with typical payload
static void bench_format_typical(benchmark::State &state) {
    simple_formatter fmt("mylogger");
    memory_buf_t buf;
    auto now = log_clock::now();
    for (auto _ : state) {
        buf.clear();
        format_line(fmt, buf, now, level::info, "Hello logger: msg number 12345...............");
        benchmark::DoNotOptimize(buf.data());
    }
}

// Format with long payload (500 bytes)
static void bench_format_long(benchmark::State &state) {
    simple_formatter fmt("mylogger");
    memory_buf_t buf;
    auto now = log_clock::now();
    string_view_t payload =
        "Lorem ipsum dolor sit amet, consectetur adipiscing elit. Vestibulum pharetra metus cursus "
        "lacus placerat congue. Nulla egestas, mauris a tincidunt tempus, enim lectus volutpat mi, "
        "eu consequat sem libero nec massa. In dapibus ipsum a diam rhoncus gravida. Etiam non dapibus eros. Donec "
        "fringilla dui sed augue pretium, nec scelerisque est maximus. Nullam convallis, sem nec blandit maximus, "
        "nisi turpis ornare nisl, sit amet volutpat neque massa eu odio. Maecenas malesuada quam ex.";
    for (auto _ : state) {
        buf.clear();
        format_line(fmt, buf, now, level::info, payload);
        benchmark::DoNotOptimize(buf.data());
    }
}

// Just the timestamp (cached path - same second)
static void bench_timestamp_cached(benchmark::State &state) {
    simple_formatter fmt("x");
    memory_buf_t buf;
    auto now = log_clock::now();
    // Warm the cache
    format_line(fmt, buf, now, level::info, "x");
    for (auto _ : state) {
        buf.clear();
        format_line(fmt, buf, now, level::info, "x");
        benchmark::DoNotOptimize(buf.data());
    }
}

// Timestamp with varying milliseconds (simulates real traffic)
static void bench_timestamp_varying_ms(benchmark::State &state) {
    simple_formatter fmt("x");
    memory_buf_t buf;
    int ms = 0;
    auto base = std::chrono::system_clock::now();
    for (auto _ : state) {
        auto t = base + std::chrono::milliseconds(ms++ % 1000);
        buf.clear();
        format_line(fmt, buf, t, level::info, "x");
        benchmark::DoNotOptimize(buf.data());
    }
}

BENCHMARK(bench_format_short);
BENCHMARK(bench_format_typical);
BENCHMARK(bench_format_long);
BENCHMARK(bench_timestamp_cached);
BENCHMARK(bench_timestamp_varying_ms);

BENCHMARK_MAIN();
