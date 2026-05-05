# CLAUDE.md

This file provides guidance to Claude Code (claude.ai/code) when working with code in this repository.

## What is spdlite

A minimal, header-only C++20 logging library - a lite version of [spdlog](https://github.com/gabime/spdlog). Sinks are compile-time template parameters (zero virtual dispatch). Thread safety is selected via `logger_mt` (mutex) vs `logger_st` (null_mutex). Bundles fmt 12.1.0 (header-only); can alternatively use `std::format` via `-DSPDLITE_USE_STD_FORMAT`.

## Build commands

```bash
# configure + build example (default)
cmake -B build .
cmake --build build

# build with std::format instead of bundled fmt
cmake -B build -DCMAKE_CXX_FLAGS="-DSPDLITE_USE_STD_FORMAT" .
cmake --build build

# build benchmarks (requires Google Benchmark - fetched automatically)
cmake -B build -DSPDLITE_BUILD_BENCH=ON .
cmake --build build

# run
./build/example          # or build/Debug/example.exe on MSVC
./build/latency          # quick benchmarks
./build/latency full     # includes multi-threaded and file I/O benchmarks
./build/formatter_bench
```

No test suite exists yet. Formatting is governed by `.clang-format` (Google base, 4-space indent, 130-col limit) - run `clang-format -i <files>` before committing.

## Architecture

All code lives under `include/spdlite/` - there is no `.cpp` compilation.

- **`common.h`** - minimal shared types (`level` enum, `log_msg`, `stack_buf<N>` used as `memory_buf_t`, `level_names`, `fwrite_bytes`). Sinks include this directly, so they don't pull in the formatter or the logger template.
- **`formatter.h`** - `simple_formatter` plus `put2`/`put3`/`put4` helpers. Produces `[YYYY-MM-DD HH:MM:SS.mmm] [name] [L] payload\n`. Caches the entire header string and patches only millis (3 bytes) and level (1 byte) per call; timestamp rebuilds only on second-boundary change.
- **`logger.h`** - public include. Pulls in `common.h` + `formatter.h` and adds the `format_string_t`/`format_args_t` aliases, `null_mutex`, `atomic_level_t`, and the `logger<Mutex, Sinks...>` template (with `logger_mt` / `logger_st` aliases). Two internal paths: `log_sv_` (string_view, no formatting) and `log_fmt_args_` (fmt/std::format into payload buffer, fed by per-Args trampoline `dispatch_fmt_`). Both consult `should_flush(lvl)` after dispatch and fold-call `flush()` on every sink when the message level meets the flush threshold (default `level::off` = no auto-flush).
- **`sinks/`** - each sink is a simple struct with `write(const log_msg&)` and `flush()`, including only `common.h` from spdlite. The `log_msg` carries metadata plus two views into the logger's shared buffer: `formatted` (the whole line, header + payload + newline) and `payload` (the raw user message, no header, no newline). Sinks typically write `msg.formatted`; structured sinks (JSON, syslog, etc.) can read `msg.payload` directly. No base class - sinks are duck-typed template parameters.
  - `stdout_sink` / `stderr_sink` - plain `fwrite` to stdout/stderr.
  - `console_sink` / `console_err_sink` - inserts color around the level tag. Native Win32 `SetConsoleTextAttribute` on Windows, ANSI escape codes on Linux/macOS.
  - `file_sink` - `fopen`/`fwrite` with RAII via `unique_ptr<FILE>`. Creates parent directories automatically.
  - `null_sink` - discards output (used in benchmarks).
- **`fmt/`** - vendored fmt 12.1.0 headers (`base.h`, `format.h`, `format-inl.h`). Do not edit these.

To drop into another project, copy `include/spdlite/logger.h` plus the sinks you need (and `fmt/` unless using `SPDLITE_USE_STD_FORMAT`). No CMake required.

## Code style

- Comments inside function bodies start lowercase; comments before declarations can start uppercase.
- Level names are fixed 1-char tags: T, D, I, W, E, C, O.
- Single namespace: `spdlite`. Sinks are types like `console_sink`, `file_sink`, `null_sink` directly in `spdlite::`.
