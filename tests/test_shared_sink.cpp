// SPDX-License-Identifier: MIT

#include <doctest/doctest.h>

#include <memory>

#include "spdlite/logger.h"
#include "spdlite/sinks/shared_sink.h"
#include "support/capture_sink.h"

using namespace spdlite;
using spdlite_test::capture_sink;

TEST_CASE("shared_sink forwards writes to the underlying sink") {
    auto inner = std::make_shared<capture_sink>();
    shared_sink<capture_sink> wrapped{inner};

    logger_st<shared_sink<capture_sink>> log{"a", wrapped};
    log.info("hello");
    log.warn("there");

    CHECK(inner->state->payloads.size() == 2);
    CHECK(inner->state->payloads[0] == "hello");
    CHECK(inner->state->payloads[1] == "there");
}

TEST_CASE("two loggers can share one underlying sink") {
    auto inner = std::make_shared<capture_sink>();
    shared_sink<capture_sink> wrapped{inner};

    logger_st<shared_sink<capture_sink>> a{"a", wrapped};
    logger_st<shared_sink<capture_sink>> b{"b", wrapped};

    a.info("from-a");
    b.info("from-b");
    a.info("from-a-2");

    REQUIRE(inner->state->payloads.size() == 3);
    CHECK(inner->state->payloads[0] == "from-a");
    CHECK(inner->state->payloads[1] == "from-b");
    CHECK(inner->state->payloads[2] == "from-a-2");

    // both loggers' headers reach the same sink
    CHECK(inner->state->formatted[0].find("[a]") != std::string::npos);
    CHECK(inner->state->formatted[1].find("[b]") != std::string::npos);
}

TEST_CASE("shared_sink copy shares the same underlying sink") {
    auto inner = std::make_shared<capture_sink>();
    shared_sink<capture_sink> a{inner};
    shared_sink<capture_sink> b = a;  // copy

    CHECK(a.get() == b.get());  // shared_ptr equality: same managed object
    CHECK(a.get() == inner);
}

TEST_CASE("shared_sink flush reaches the underlying sink") {
    auto inner = std::make_shared<capture_sink>();
    shared_sink<capture_sink> wrapped{inner};
    wrapped.flush();
    wrapped.flush();
    CHECK(inner->state->flush_count == 2);
}
