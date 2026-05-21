// SPDX-License-Identifier: MIT

// At LEVEL_TRACE, all six macros must emit and evaluate their args.

// SPDLITE_LEVEL_TRACE is defined inside logger.h. Lazy preprocessor expansion
// makes this work — the replacement text is only evaluated at the #if checks
// below logger.h's level constant definitions. See test_log_macros_warn.cpp.
#define SPDLITE_ACTIVE_LEVEL SPDLITE_LEVEL_TRACE

#include <doctest/doctest.h>

#include "helpers.h"
#include "spdlite/logger.h"

using namespace spdlite;
using helpers::capture_sink;

TEST_CASE("at LEVEL_TRACE, all six macros emit and evaluate their args") {
    int call_count = 0;
    auto bumper = [&] {
        ++call_count;
        return 42;
    };
    capture_sink cap;
    logger_st<capture_sink> log{cap};
    log.set_log_level(level::trace);

    SPDLITE_TRACE(log, "x={}", bumper());
    SPDLITE_DEBUG(log, "x={}", bumper());
    SPDLITE_INFO(log, "x={}", bumper());
    SPDLITE_WARN(log, "x={}", bumper());
    SPDLITE_ERROR(log, "x={}", bumper());
    SPDLITE_CRITICAL(log, "x={}", bumper());

    CHECK(call_count == 6);
    CHECK(cap.state->payloads.size() == 6);
}
