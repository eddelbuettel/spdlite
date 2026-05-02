// spdlite vs spdlog comparative benchmark

#include <cstdio>
#include <iostream>
#include <map>
#include <string>
#include <vector>

#include "benchmark/benchmark.h"

// spdlite
#include "spdlite/spdlite.h"
#include "spdlite/sinks/basic_file_sink.h"
#include "spdlite/sinks/null_sink.h"
#include "spdlite/sinks/color_sink.h"

// spdlog
#include "spdlog/version.h"
#include "spdlog/logger.h"
#include "spdlog/sinks/basic_file_sink.h"
#include "spdlog/sinks/null_sink.h"
#include "spdlog/sinks/stdout_color_sinks.h"

// --- disabled at runtime ---

static void spdlite_disabled(benchmark::State& state) {
    spdlite::logger_st<spdlite::sinks::null_sink> log("bench");
    log.log_level(spdlite::level::off);
    int i = 0;
    for (auto _ : state) {
        log.info("Hello logger: msg number {}...............", ++i);
    }
}

static void spdlog_disabled(benchmark::State& state) {
    auto sink = std::make_shared<spdlog::sinks::null_sink_st>();
    spdlog::logger log("bench", sink);
    log.set_level(spdlog::level::off);
    int i = 0;
    for (auto _ : state) {
        log.info("Hello logger: msg number {}...............", ++i);
    }
}

// --- null sink, formatted (single-threaded) ---

static void spdlite_null_fmt_st(benchmark::State& state) {
    spdlite::logger_st<spdlite::sinks::null_sink> log("bench");
    int i = 0;
    for (auto _ : state) {
        log.info("Hello logger: msg number {}...............", ++i);
    }
}

static void spdlog_null_fmt_st(benchmark::State& state) {
    auto sink = std::make_shared<spdlog::sinks::null_sink_st>();
    spdlog::logger log("bench", sink);
    int i = 0;
    for (auto _ : state) {
        log.info("Hello logger: msg number {}...............", ++i);
    }
}

// --- null sink, long string (no formatting) ---

static const char* long_msg =
    "Lorem ipsum dolor sit amet, consectetur adipiscing elit. Vestibulum pharetra metus cursus "
    "lacus placerat congue. Nulla egestas, mauris a tincidunt tempus, enim lectus volutpat mi, "
    "eu consequat sem libero nec massa. In dapibus ipsum a diam rhoncus gravida. Etiam non dapibus eros. Donec "
    "fringilla dui sed augue pretium, nec scelerisque est maximus. Nullam convallis, sem nec blandit maximus, "
    "nisi turpis ornare nisl, sit amet volutpat neque massa eu odio. Maecenas malesuada quam ex.";

static void spdlite_null_cstr_st(benchmark::State& state) {
    spdlite::logger_st<spdlite::sinks::null_sink> log("bench");
    for (auto _ : state) {
        log.info(long_msg);
    }
}

static void spdlog_null_cstr_st(benchmark::State& state) {
    auto sink = std::make_shared<spdlog::sinks::null_sink_st>();
    spdlog::logger log("bench", sink);
    for (auto _ : state) {
        log.info(long_msg);
    }
}

// --- color stdout (single-threaded) ---

static void spdlite_color_st(benchmark::State& state) {
    spdlite::logger_st<spdlite::sinks::color_stdout> log("bench");
    int i = 0;
    for (auto _ : state) {
        log.info("Hello logger: msg number {}...............", ++i);
    }
}

static void spdlog_color_st(benchmark::State& state) {
    auto sink = std::make_shared<spdlog::sinks::stdout_color_sink_st>();
    spdlog::logger log("bench", sink);
    int i = 0;
    for (auto _ : state) {
        log.info("Hello logger: msg number {}...............", ++i);
    }
}

// --- color stdout (multi-threaded) ---

static void spdlite_color_mt(benchmark::State& state) {
    static spdlite::logger_mt<spdlite::sinks::color_stdout> log("bench");
    int i = 0;
    for (auto _ : state) {
        log.info("Hello logger: msg number {}...............", ++i);
    }
}

static void spdlog_color_mt(benchmark::State& state) {
    static auto log = [] {
        auto sink = std::make_shared<spdlog::sinks::stdout_color_sink_mt>();
        return spdlog::logger("bench", sink);
    }();
    int i = 0;
    for (auto _ : state) {
        log.info("Hello logger: msg number {}...............", ++i);
    }
}

// --- basic file sink (single-threaded) ---

#ifdef _WIN32
static constexpr const char* null_file = "NUL";
#else
static constexpr const char* null_file = "/dev/null";
#endif

static void spdlite_file_st(benchmark::State& state) {
    spdlite::logger_st<spdlite::sinks::basic_file_sink> log("bench",
                                                            spdlite::sinks::basic_file_sink{null_file, true});
    int i = 0;
    for (auto _ : state) {
        log.info("Hello logger: msg number {}...............", ++i);
    }
}

static void spdlog_file_st(benchmark::State& state) {
    auto sink = std::make_shared<spdlog::sinks::basic_file_sink_st>(null_file, true);
    spdlog::logger log("bench", sink);
    int i = 0;
    for (auto _ : state) {
        log.info("Hello logger: msg number {}...............", ++i);
    }
}

// --- basic file sink (multi-threaded) ---

static void spdlite_file_mt(benchmark::State& state) {
    static spdlite::logger_mt<spdlite::sinks::basic_file_sink> log(
        "bench", spdlite::sinks::basic_file_sink{null_file, true});
    int i = 0;
    for (auto _ : state) {
        log.info("Hello logger: msg number {}...............", ++i);
    }
}

static void spdlog_file_mt(benchmark::State& state) {
    static auto log = [] {
        auto sink = std::make_shared<spdlog::sinks::basic_file_sink_mt>(null_file, true);
        return spdlog::logger("bench", sink);
    }();
    int i = 0;
    for (auto _ : state) {
        log.info("Hello logger: msg number {}...............", ++i);
    }
}

// --- result collector for comparison table ---

struct result_collector : benchmark::ConsoleReporter {
    std::vector<Run> all_runs;

    void ReportRuns(const std::vector<Run>& report) override {
        ConsoleReporter::ReportRuns(report);
        for (const auto& r : report) all_runs.push_back(r);
    }
};

static void print_comparison(const std::vector<benchmark::BenchmarkReporter::Run>& runs) {
    struct row {
        std::string test;
        double spdlite_ns = 0;
        double spdlog_ns = 0;
    };

    std::vector<row> table;
    std::map<std::string, size_t> index;

    for (const auto& r : runs) {
        std::string name = r.run_name.function_name;
        std::string key;
        bool is_lite = false;

        if (name.compare(0, 8, "spdlite_") == 0) {
            key = name.substr(8);
            is_lite = true;
        } else if (name.compare(0, 7, "spdlog_") == 0) {
            key = name.substr(7);
        } else {
            continue;
        }

        auto it = index.find(key);
        if (it == index.end()) {
            index[key] = table.size();
            table.push_back({key, 0.0, 0.0});
            it = index.find(key);
        }

        double ns = r.real_accumulated_time / static_cast<double>(r.iterations) * 1e9;
        if (is_lite)
            table[it->second].spdlite_ns = ns;
        else
            table[it->second].spdlog_ns = ns;
    }

    // print to stderr (stdout is redirected to null)
    fprintf(stderr, "\nspdlite version: %d.%d.%d\n", SPDLITE_VER_MAJOR, SPDLITE_VER_MINOR, SPDLITE_VER_PATCH);
    fprintf(stderr, "spdlog version:  %d.%d.%d\n", SPDLOG_VER_MAJOR, SPDLOG_VER_MINOR, SPDLOG_VER_PATCH);
    fprintf(stderr, "====================== spdlite vs spdlog ======================\n");
    fprintf(stderr, "%-20s %13s %13s %9s\n", "Test", "spdlog", "spdlite", "speedup");
    fprintf(stderr, "----------------------------------------------------------------\n");
    for (const auto& r : table) {
        if (r.spdlite_ns > 0 && r.spdlog_ns > 0) {
            double ratio = r.spdlog_ns / r.spdlite_ns;
            fprintf(stderr, "%-20s %10.1f ns %10.1f ns %7.2fx\n", r.test.c_str(), r.spdlog_ns, r.spdlite_ns, ratio);
        }
    }
    fprintf(stderr, "================================================================\n");
}

int main(int argc, char* argv[]) {
    int n_threads = 4;
    auto full_bench = argc > 1 && std::string(argv[1]) == "full";

    benchmark::RegisterBenchmark("spdlite_disabled", spdlite_disabled);
    benchmark::RegisterBenchmark("spdlog_disabled", spdlog_disabled);

    benchmark::RegisterBenchmark("spdlite_null_fmt_st", spdlite_null_fmt_st);
    benchmark::RegisterBenchmark("spdlog_null_fmt_st", spdlog_null_fmt_st);

    benchmark::RegisterBenchmark("spdlite_null_cstr_st", spdlite_null_cstr_st);
    benchmark::RegisterBenchmark("spdlog_null_cstr_st", spdlog_null_cstr_st);

    benchmark::RegisterBenchmark("spdlite_file_st", spdlite_file_st)->UseRealTime();
    benchmark::RegisterBenchmark("spdlog_file_st", spdlog_file_st)->UseRealTime();

    benchmark::RegisterBenchmark("spdlite_file_mt_1t", spdlite_file_mt)->UseRealTime();
    benchmark::RegisterBenchmark("spdlog_file_mt_1t", spdlog_file_mt)->UseRealTime();

    benchmark::RegisterBenchmark("spdlite_file_mt", spdlite_file_mt)->Threads(n_threads)->UseRealTime();
    benchmark::RegisterBenchmark("spdlog_file_mt", spdlog_file_mt)->Threads(n_threads)->UseRealTime();

    if (full_bench) {
        benchmark::RegisterBenchmark("spdlite_color_st", spdlite_color_st)->UseRealTime();
        benchmark::RegisterBenchmark("spdlog_color_st", spdlog_color_st)->UseRealTime();

        benchmark::RegisterBenchmark("spdlite_color_mt", spdlite_color_mt)->Threads(n_threads)->UseRealTime();
        benchmark::RegisterBenchmark("spdlog_color_mt", spdlog_color_mt)->Threads(n_threads)->UseRealTime();
    }

    benchmark::Initialize(&argc, argv);

    // redirect C stdout to null so log output from stdout/color sinks is silenced,
    // but keep benchmark results visible via cout -> stderr
    std::ios::sync_with_stdio(false);
    std::cout.rdbuf(std::cerr.rdbuf());
#ifdef _WIN32
    if (std::freopen("NUL", "w", stdout) == nullptr) {
        std::perror("freopen failed");
        return 1;
    }
#else
    if (std::freopen("/dev/null", "w", stdout) == nullptr) {
        std::perror("freopen failed");
        return 1;
    }
#endif

    result_collector collector;
    benchmark::RunSpecifiedBenchmarks(&collector);

    print_comparison(collector.all_runs);
}
