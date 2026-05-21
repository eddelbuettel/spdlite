// SPDX-License-Identifier: MIT

// At LEVEL_OFF, all six macros must elide entirely — zero arg evaluation,
// zero messages reaching the sink.

// SPDLITE_LEVEL_OFF is defined inside logger.h. Lazy preprocessor expansion
// makes this work — see test_log_macros_warn.cpp for the explanation.
#define SPDLITE_ACTIVE_LEVEL SPDLITE_LEVEL_OFF

#include <doctest/doctest.h>

#include "helpers.h"
#include "spdlite/logger.h"

using namespace spdlite;
using helpers::capture_sink;

TEST_CASE("at LEVEL_OFF, all six macros elide entirely") {
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

    CHECK(call_count == 0);
    CHECK(cap.state->payloads.empty());
}
