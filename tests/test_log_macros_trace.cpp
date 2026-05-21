// SPDX-License-Identifier: MIT

// LEVEL_TRACE: all six macros emit and evaluate their args.
#define SPDLITE_ACTIVE_LEVEL SPDLITE_LEVEL_TRACE  // lazily expanded inside logger.h

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
