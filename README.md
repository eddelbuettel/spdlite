# spdlite

A small, header-only C++20 logger - the lite version of [spdlog](https://github.com/gabime/spdlog), simpler, smaller, fewer features.

Use spdlite if you want a tiny, fast, capable logger.

## Install

Just copy the `include/spdlite/` folder into your build tree.

## Quick start
```c++
#include "spdlite/logger.h"
#include "spdlite/sinks/color_sink.h"

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
[2026-04-11 10:30:45.123] [app] [I] Hello world
[2026-04-11 10:30:45.123] [app] [I] Value: 42
[2026-04-11 10:30:45.123] [app] [W] Something happened
[2026-04-11 10:30:45.123] [app] [E] Failed with code -1
```

See [`include/spdlite/logger.h`](include/spdlite/logger.h) for the full API and [`include/spdlite/sinks/`](include/spdlite/sinks/) for the available sinks.

## fmt vs `std::format`

If you'd rather use `<format>` (zero vendored dependencies, smaller install),
define `SPDLITE_USE_STD_FORMAT` and the `fmt/` subfolder can be left out:

```console
$ cmake -DCMAKE_CXX_FLAGS="-DSPDLITE_USE_STD_FORMAT" ..
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
