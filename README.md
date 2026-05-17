# spdlite

A small, header-only C++20 logger - the lite version of [spdlog](https://github.com/gabime/spdlog), simpler, smaller, fewer features.

Use spdlite if you want a tiny, fast, capable logger.

## Install

Just copy the `include/spdlite/` folder into your build tree.

## Quick start
```c++
#include "spdlite/logger.h"
#include "spdlite/sinks/console_sink.h"

int main() {
    using namespace spdlite;
    logger_st<console_sink> log("app", console_sink{});

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

See [`include/spdlite/logger.h`](include/spdlite/logger.h) for the full API and [`include/spdlite/sinks/`](include/spdlite/sinks/) for the available sinks.

## fmt vs `std::format`

If you'd rather use `<format>` (zero vendored dependencies, smaller install),
define `SPDLITE_USE_STD_FORMAT` and the `fmt/` subfolder can be left out:

```console
$ cmake -DCMAKE_CXX_FLAGS="-DSPDLITE_USE_STD_FORMAT" ..
```

## Formatter options

The default header is `[YYYY-MM-DD HH:MM:SS.mmm] [name] [LVL] payload`. Reconfigure
via `format_options`:

```c++
log.format_options({.utc = true});
log.format_options({.precision = time_precision::ns});
log.format_options({.show_date = false, .precision = time_precision::none});
```

See the table below for all available fields:

| Field            | Default              | Effect                                                       |
| ---------------- | -------------------- | ------------------------------------------------------------ |
| `utc`            | `false`              | Use `gmtime` instead of `localtime`.                         |
| `show_date`      | `true`               | Include the `YYYY-MM-DD ` prefix.                            |
| `show_thread_id` | `false`              | Include a 6-digit thread id after the timestamp.             |
| `precision`      | `time_precision::ms` | Fractional digits: `none`, `ms` (3), `us` (6), or `ns` (9).  |

## Benchmarks

Build and run:
```console
$ cmake -B build -DSPDLITE_BUILD_BENCH=ON .
$ cmake --build build
$ ./build/latency           # quick set
$ ./build/latency full      # adds multi-threaded + file I/O
```

## License
MIT
