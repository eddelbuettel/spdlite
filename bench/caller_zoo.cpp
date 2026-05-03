// Synthetic stress for measuring per-callsite cost of the templated logging API.
// Uses Tag<N> to mint dozens of unique types via macro repetition, so each
// log call site has a distinct Args... pack - the worst case for template
// instantiation bloat. ~83 unique packs × 6 levels = ~498 log calls.
//
// Build: cmake -B build -DSPDLITE_BUILD_CALLER_ZOO=ON .
//        cmake --build build --config Release
// Compare:
//   - .text size: dumpbin /headers caller_zoo.exe | findstr text
//   - compile time: time the cmake --build invocation
//
// Note: fmt-mode only. The Tag<N> formatter specializes fmt::formatter,
// not std::formatter - building with -DSPDLITE_USE_STD_FORMAT will fail.

#include "spdlite/spdlite.h"
#include "spdlite/sinks/stdout_sink.h"

#include <cstdint>
#include <string>
#include <string_view>

#ifdef SPDLITE_USE_STD_FORMAT
#error "caller_zoo requires the fmt path (uses fmt::formatter specialization for Tag<N>)"
#endif

// Tag<N> is a distinct type per N. Each unique N produces a unique Args... pack.
template <int N>
struct Tag {
    int v = N;
};

template <int N>
struct fmt::formatter<Tag<N>> : fmt::formatter<int> {
    auto format(Tag<N> t, fmt::format_context& ctx) const {
        return fmt::formatter<int>::format(t.v, ctx);
    }
};

// Expand to all six per-level methods so each unique Args... pack costs 6 instantiations.
#define ZOO_ALL_LEVELS(fmt_str, ...)                  \
    do {                                              \
        log.trace(fmt_str, __VA_ARGS__);              \
        log.debug(fmt_str, __VA_ARGS__);              \
        log.info(fmt_str, __VA_ARGS__);               \
        log.warn(fmt_str, __VA_ARGS__);               \
        log.error(fmt_str, __VA_ARGS__);              \
        log.critical(fmt_str, __VA_ARGS__);           \
    } while (0)

// One-arg, two-arg, three-arg call generators parameterized by an integer seed.
// Each produces a distinct Args... pack because Tag<N> is a unique type per N.
#define ONE(N)   ZOO_ALL_LEVELS("u{}: {}",       N, Tag<(N)>{});
#define TWO(N)   ZOO_ALL_LEVELS("d{}: {} {}",    N, Tag<(N)>{}, Tag<(N) + 10000>{});
#define THREE(N) ZOO_ALL_LEVELS("t{}: {} {} {}", N, Tag<(N)>{}, Tag<(N) + 20000>{}, Tag<(N) + 30000>{});

// Repetition primitives - preprocessor doesn't do arithmetic, but the C++
// compiler evaluates `(N) + k` as a constant when stamping out Tag<...>,
// so each expansion below produces a distinct integer template argument.
#define R10(M, N)                                      \
    M(N + 0) M(N + 1) M(N + 2) M(N + 3) M(N + 4)       \
    M(N + 5) M(N + 6) M(N + 7) M(N + 8) M(N + 9)
#define R50(M, N)                                                            \
    R10(M, (N) + 0) R10(M, (N) + 10) R10(M, (N) + 20) R10(M, (N) + 30) R10(M, (N) + 40)
#define R100(M, N) R50(M, (N) + 0) R50(M, (N) + 50)
#define R500(M, N) R100(M, (N) + 0) R100(M, (N) + 100) R100(M, (N) + 200) R100(M, (N) + 300) R100(M, (N) + 400)

int main() {
    using namespace spdlite;
    // stdout_sink + level::trace: every call actually formats and writes at
    // runtime, so the compiler cannot prove the bodies dead via the level check.
    // (We don't run this binary for the measurement - only build it.)
    logger_st<stdout_sink> log;
    log.log_level(level::trace);

    // 50 unique 1-arg packs × 6 levels = 300 calls
    R50(ONE, 0)

    // 10 unique 2-arg packs × 6 levels = 60 calls
    R10(TWO, 0)

    // 10 unique 3-arg packs × 6 levels = 60 calls
    R10(THREE, 0)

    // 13 mixed-builtin-type packs × 6 levels = 78 calls - plus the 420 above ≈ 498 total
    // (the realistic part - actual user types and arities)
    const std::string s{"hello"};
    const std::string_view sv{"world"};
    ZOO_ALL_LEVELS("m01 {}", static_cast<int>(1));
    ZOO_ALL_LEVELS("m02 {}", static_cast<long>(1));
    ZOO_ALL_LEVELS("m03 {}", static_cast<double>(1.0));
    ZOO_ALL_LEVELS("m04 {}", "literal");
    ZOO_ALL_LEVELS("m05 {}", s);
    ZOO_ALL_LEVELS("m06 {}", sv);
    ZOO_ALL_LEVELS("m07 {} {}", 1, 2L);
    ZOO_ALL_LEVELS("m08 {} {}", 1, "x");
    ZOO_ALL_LEVELS("m09 {} {}", "x", s);
    ZOO_ALL_LEVELS("m10 {} {}", s, sv);
    ZOO_ALL_LEVELS("m11 {} {} {}", 1, 2L, 3.0);
    ZOO_ALL_LEVELS("m12 {} {} {} {}", 1, 2L, 3.0, "x");
    ZOO_ALL_LEVELS("m13 {} {} {} {} {}", 1, 2L, 3.0, "x", s);

    return 0;
}
