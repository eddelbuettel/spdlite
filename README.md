# spdlite

Minimal, very lite version of [spdlog](https://github.com/gabime/spdlog).

## Features
* Header-only. Copy the `include/spdlite` folder and go.
* Zero virtual dispatch — sinks are compile-time template parameters.
* Thread-safe — `logger_mt` / `logger_st` variants.
* Bundled [fmt](https://github.com/fmtlib/fmt) 12.1.0 (3 headers). Or use `std::format` with zero dependencies.

## Quick start
```c++
#include "spdlite/logger.h"
#include "spdlite/sinks/stdout_color_sink.h"

int main() {
    using namespace spdlite;
    logger_st<sinks::stdout_color_sink> log("app");

    log.info("Hello {}", "world");
    log.info("Value: {}", 42);
    log.warn("Something happened");
    log.error("Failed with code {}", -1);
}
```

Output:
```
[2026-04-11 10:30:45.123] [app] [INF] Hello world
[2026-04-11 10:30:45.123] [app] [INF] Value: 42
[2026-04-11 10:30:45.123] [app] [WRN] Something happened
[2026-04-11 10:30:45.123] [app] [ERR] Failed with code -1
```

## Sinks

| Sink | Header | Description |
|------|--------|-------------|
| `stdout_sink` | `sinks/stdout_sink.h` | Write to stdout |
| `stderr_sink` | `sinks/stdout_sink.h` | Write to stderr |
| `stdout_color_sink` | `sinks/stdout_color_sink.h` | Colored stdout (ANSI) |
| `stderr_color_sink` | `sinks/stdout_color_sink.h` | Colored stderr (ANSI) |
| `basic_file_sink` | `sinks/basic_file_sink.h` | Write to file |
| `null_sink` | `sinks/null_sink.h` | Discard output |

## Multiple sinks
```c++
using namespace spdlite;
logger_st<sinks::stdout_color_sink, sinks::basic_file_sink> log(
    "app",
    sinks::stdout_color_sink{},
    sinks::basic_file_sink{"logs/app.txt"});

log.info("Color on console, plain text in file");
```

## Thread safety
```c++
// multi-threaded — logger locks once per log call
logger_mt<sinks::basic_file_sink> log("app", sinks::basic_file_sink{"app.log"});

// single-threaded — zero locking overhead
logger_st<sinks::stdout_sink> log("app");
```

## Log levels
```c++
log.set_level(level::trace);  // show all messages
log.set_level(level::warn);   // show only warn, error, critical
```

Levels: `trace`, `debug`, `info`, `warn`, `err`, `critical`, `off`.

## Using std::format instead of fmt
Define `SPDLITE_USE_STD_FORMAT` before including any spdlite header (or pass as compiler flag). This removes the fmt dependency entirely — only C++20 standard library is used. Slightly slower on MSVC (~1.3x), but zero external dependencies.

```console
$ cmake -DCMAKE_CXX_FLAGS="-DSPDLITE_USE_STD_FORMAT" ..
```

## Nameless logger
```c++
logger_st<sinks::stdout_color_sink> log;
log.info("No name in the output");
```
```
[2026-04-11 10:30:45.123] [INF] No name in the output
```

## Benchmarks

Build and run the comparative benchmark against spdlog:
```console
$ cmake -B build -DSPDLITE_BUILD_BENCH=ON .
$ cmake --build build --config Release
$ ./build/Release/vs_spdlog        # quick benchmarks
$ ./build/Release/vs_spdlog full   # includes multi-threaded and color sink benchmarks
```

Sample output (Windows, MSVC, Release):
```
spdlite version: 0.1.0
spdlog version:  1.17.0
====================== spdlite vs spdlog ======================
Test                        spdlog       spdlite   speedup
----------------------------------------------------------------
disabled                    8.3 ns        0.2 ns   36.34x
null_fmt_st                65.2 ns       69.2 ns    0.94x
null_cstr_st              104.5 ns       70.8 ns    1.47x
file_st                   112.1 ns      108.4 ns    1.03x
file_mt_1t                127.3 ns      117.1 ns    1.09x
file_mt                   277.6 ns      190.9 ns    1.45x
================================================================
```

## Build
```console
$ cmake -B build .
$ cmake --build build
```

Or just copy `include/spdlite/` into your project. No build step needed.

## License
MIT
