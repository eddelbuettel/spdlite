# spdlite

A small, header-only C++20 logger - the lite version of [spdlog](https://github.com/gabime/spdlog), simpler, smaller, fewer features.

## Install

Just copy the `include/spdlite/` folder into to your build tree.

## Quick start
```c++
#include "spdlite/spdlite.h"
#include "spdlite/sinks/color_sink.h"

int main() {
    using namespace spdlite;
    logger_st<console_sink> log("app");

    log.info("Hello {}", "world");
    log.info("Value: {}", 42);
    log.warn("Something happened");
    log.error("Failed with code {}", -1);
}
```

Output:
```
[2026-04-11 10:30:45.123] [app] [I] Hello world
[2026-04-11 10:30:45.123] [app] [I] Value: 42
[2026-04-11 10:30:45.123] [app] [W] Something happened
[2026-04-11 10:30:45.123] [app] [E] Failed with code -1
```

## Sinks

| Sink | Header | Description |
|------|--------|-------------|
| `stdout_sink` | `sinks/stdout_sink.h` | Write to stdout |
| `stderr_sink` | `sinks/stdout_sink.h` | Write to stderr |
| `console_sink` | `sinks/color_sink.h` | Colored stdout (Win32 API on Windows, ANSI on Linux/macOS) |
| `console_err_sink` | `sinks/color_sink.h` | Colored stderr |
| `file_sink` | `sinks/file_sink.h` | Write to file |
| `null_sink` | `sinks/null_sink.h` | Discard output |

## Multiple sinks
```c++
using namespace spdlite;
logger_st<console_sink, file_sink> log(
    "app",
    console_sink{},
    file_sink{"logs/app.txt"});

log.info("Color on console, plain text in file");
```

## Thread safety
```c++
// multi-threaded - logger locks once per log call
logger_mt<file_sink> log("app", file_sink{"app.log"});

// single-threaded - zero locking overhead
logger_st<stdout_sink> log("app");
```

## Log levels
```c++
log.log_level(level::trace);  // show all messages
log.log_level(level::warn);   // show only warn, error, critical
```

Levels: `trace`, `debug`, `info`, `warn`, `err`, `critical`, `off`.

## Flush level
By default, sinks are only flushed when you call `log.flush()` explicitly. Set a flush level to auto-flush on messages at or above a given severity:
```c++
log.flush_level(level::warn);  // auto-flush on warn, error, critical
log.flush_level(level::off);   // disable auto-flush (default)
```

## fmt vs `std::format`

If you'd rather use `<format>` (zero vendored dependencies, smaller install),
define `SPDLITE_USE_STD_FORMAT` and the `fmt/` subfolder can be left out:

```console
$ cmake -DCMAKE_CXX_FLAGS="-DSPDLITE_USE_STD_FORMAT" ..
```

## Nameless logger
```c++
logger_st<console_sink> log;
log.info("No name in the output");
```
```
[2026-04-11 10:30:45.123] [I] No name in the output
```

## Benchmarks

Build and run the comparative benchmark against spdlog:
```console
$ cmake -B build -DSPDLITE_BUILD_BENCH=ON .
$ cmake --build build --config Release
$ ./build/Release/vs_spdlog        # quick benchmarks
$ ./build/Release/vs_spdlog full   # includes multi-threaded and color sink benchmarks
```

Sample output (Intel Core Ultra 7 255H, Windows, MSVC, Release):
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

## License
MIT
