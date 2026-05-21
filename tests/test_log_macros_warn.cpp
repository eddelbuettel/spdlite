// SPDX-License-Identifier: MIT

// At LEVEL_WARN, TRACE/DEBUG/INFO must elide (zero arg evaluation),
// WARN/ERROR/CRITICAL must emit. This is the boundary correctness witness.

// SPDLITE_LEVEL_WARN is defined inside logger.h. This works because the
// preprocessor expands SPDLITE_ACTIVE_LEVEL lazily — its replacement text
// is only evaluated at the #if comparisons inside logger.h, which run
// after the SPDLITE_LEVEL_* constants are defined there.
#define SPDLITE_ACTIVE_LEVEL SPDLITE_LEVEL_WARN

#include <doctest/doctest.h>

#include "helpers.h"
#include "spdlite/logger.h"

using namespace spdlite;
using helpers::capture_sink;

TEST_CASE("at LEVEL_WARN, trace/debug/info elide; warn/error/critical emit") {
    int call_count = 0;
    auto bumper = [&] {
        ++call_count;
        return 42;
    };
    capture_sink cap;
    logger_st<capture_sink> log{cap};
    log.set_log_level(level::trace);  // runtime gate wide open — only the compile-time gate filters

    SPDLITE_TRACE(log, "x={}", bumper());     // elided
    SPDLITE_DEBUG(log, "x={}", bumper());     // elided
    SPDLITE_INFO(log, "x={}", bumper());      // elided
    SPDLITE_WARN(log, "x={}", bumper());      // emits
    SPDLITE_ERROR(log, "x={}", bumper());     // emits
    SPDLITE_CRITICAL(log, "x={}", bumper());  // emits

    CHECK(call_count == 3);                  // bumper invoked 3 times, not 6
    CHECK(cap.state->payloads.size() == 3);  // 3 messages reached the sink
    CHECK(cap.state->levels[0] == level::warn);
    CHECK(cap.state->levels[1] == level::err);
    CHECK(cap.state->levels[2] == level::critical);
}
