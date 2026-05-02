//
// Copyright(c) 2018 Gabi Melman.
// Distributed under the MIT License (http://opensource.org/licenses/MIT)
//

//
// latency.cpp : spdlite latency benchmarks
//

#include <cstdio>
#include <iostream>

#include "benchmark/benchmark.h"
#include "spdlite/spdlite.h"
#include "spdlite/sinks/simple_file_sink.h"
#include "spdlite/sinks/null_sink.h"
#include "spdlite/sinks/color_sink.h"

using namespace spdlite;

// Bench with a long C string (no formatting)
static void bench_null_sink_c_string(benchmark::State& state) {
    logger_st<sinks::null_sink> log("bench");
    const char* msg =
        "Lorem ipsum dolor sit amet, consectetur adipiscing elit. Vestibulum pharetra metus cursus "
        "lacus placerat congue. Nulla egestas, mauris a tincidunt tempus, enim lectus volutpat mi, "
        "eu consequat sem "
        "libero nec massa. In dapibus ipsum a diam rhoncus gravida. Etiam non dapibus eros. Donec "
        "fringilla dui sed "
        "augue pretium, nec scelerisque est maximus. Nullam convallis, sem nec blandit maximus, "
        "nisi turpis ornare "
        "nisl, sit amet volutpat neque massa eu odio. Maecenas malesuada quam ex, posuere congue "
        "nibh turpis duis.";

    for (auto _ : state) {
        log.info(msg);
    }
}

// Bench with fmt formatting
static void bench_null_sink_formatted(benchmark::State& state) {
    logger_st<sinks::null_sink> log("bench");
    int i = 0;
    for (auto _ : state) {
        log.info("Hello logger: msg number {}...............", ++i);
    }
}

// Bench with logger disabled at runtime
static void bench_disabled_runtime(benchmark::State& state) {
    logger_st<sinks::null_sink> log("bench");
    log.log_level(level::off);
    int i = 0;
    for (auto _ : state) {
        log.info("Hello logger: msg number {}...............", ++i);
    }
}

// Bench null_sink_mt with multiple threads
static void bench_null_sink_mt(benchmark::State& state) {
    static logger_mt<sinks::null_sink> log("bench");
    int i = 0;
    for (auto _ : state) {
        log.info("Hello logger: msg number {}...............", ++i);
    }
}

// Bench color stdout sink (single-threaded)
static void bench_color_sink_st(benchmark::State& state) {
    logger_st<sinks::color_stdout> log("bench");
    int i = 0;
    for (auto _ : state) {
        log.info("Hello logger: msg number {}...............", ++i);
    }
}

// Bench color stdout sink (multi-threaded)
static void bench_color_sink_mt(benchmark::State& state) {
    static logger_mt<sinks::color_stdout> log("bench");
    int i = 0;
    for (auto _ : state) {
        log.info("Hello logger: msg number {}...............", ++i);
    }
}

// Bench basic file sink (single-threaded)
static void bench_basic_file_st(benchmark::State& state) {
    logger_st<sinks::simple_file_sink> log("bench", sinks::simple_file_sink{"latency_logs/basic_st.log", true});
    int i = 0;
    for (auto _ : state) {
        log.info("Hello logger: msg number {}...............", ++i);
    }
}

// Bench basic file sink (multi-threaded)
static void bench_basic_file_mt(benchmark::State& state) {
    static logger_mt<sinks::simple_file_sink> log("bench", sinks::simple_file_sink{"latency_logs/basic_mt.log", true});
    int i = 0;
    for (auto _ : state) {
        log.info("Hello logger: msg number {}...............", ++i);
    }
}

// Bench just the formatter (no I/O)
static void bench_formatter_only(benchmark::State& state) {
    simple_formatter formatter("bench");
    memory_buf_t buf;
    auto now = log_clock::now();
    string_view_t payload = "Hello logger: msg number 12345...............";
    for (auto _ : state) {
        buf.clear();
        formatter.format_header(now, level::info, buf);
        buf.append(payload.data(), payload.data() + payload.size());
#ifdef _WIN32
        buf.push_back('\r');
#endif
        buf.push_back('\n');
        benchmark::DoNotOptimize(buf.data());
    }
}

int main(int argc, char* argv[]) {
    int n_threads = benchmark::CPUInfo::Get().num_cpus;

    auto full_bench = argc > 1 && std::string(argv[1]) == "full";

    benchmark::RegisterBenchmark("disabled-at-runtime", bench_disabled_runtime);
    benchmark::RegisterBenchmark("null_sink_st (500_bytes c_str)", bench_null_sink_c_string);
    benchmark::RegisterBenchmark("null_sink_st", bench_null_sink_formatted);
    benchmark::RegisterBenchmark("formatter_only", bench_formatter_only);
    benchmark::RegisterBenchmark("color_sink_st", bench_color_sink_st)->UseRealTime();

    if (full_bench) {
        benchmark::RegisterBenchmark("null_sink_mt", bench_null_sink_mt)->Threads(n_threads)->UseRealTime();
        benchmark::RegisterBenchmark("color_sink_mt", bench_color_sink_mt)->Threads(n_threads)->UseRealTime();
        benchmark::RegisterBenchmark("basic_file_st", bench_basic_file_st)->UseRealTime();
        benchmark::RegisterBenchmark("basic_file_mt", bench_basic_file_mt)->Threads(n_threads)->UseRealTime();
    }

    benchmark::Initialize(&argc, argv);

    // redirect C stdout to null so log output from stdout/color sinks is silenced,
    // but keep benchmark results visible via cout -> stderr
    std::ios::sync_with_stdio(false);
    std::cout.rdbuf(std::cerr.rdbuf());
#ifdef _WIN32
    (void)std::freopen("NUL", "w", stdout);
#else
    (void)std::freopen("/dev/null", "w", stdout);
#endif

    benchmark::RunSpecifiedBenchmarks();
}
