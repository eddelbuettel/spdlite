// SPDX-License-Identifier: MIT

#include <doctest/doctest.h>

#include <chrono>
#include <string>
#include <string_view>

#include "spdlite/formatter.h"

using namespace spdlite;
using namespace std::chrono;

TEST_CASE("put2 writes a zero-padded 2-digit number") {
    char buf[2];
    put2(buf, 0);
    CHECK(std::string_view(buf, 2) == "00");
    put2(buf, 7);
    CHECK(std::string_view(buf, 2) == "07");
    put2(buf, 42);
    CHECK(std::string_view(buf, 2) == "42");
    put2(buf, 99);
    CHECK(std::string_view(buf, 2) == "99");
}

TEST_CASE("put3 writes a zero-padded 3-digit number") {
    char buf[3];
    put3(buf, 0);
    CHECK(std::string_view(buf, 3) == "000");
    put3(buf, 7);
    CHECK(std::string_view(buf, 3) == "007");
    put3(buf, 999);
    CHECK(std::string_view(buf, 3) == "999");
}

TEST_CASE("put4 writes a zero-padded 4-digit number") {
    char buf[4];
    put4(buf, 0);
    CHECK(std::string_view(buf, 4) == "0000");
    put4(buf, 2026);
    CHECK(std::string_view(buf, 4) == "2026");
    put4(buf, 9999);
    CHECK(std::string_view(buf, 4) == "9999");
}

// Helper: extract the entire produced header as a string.
static std::string format_one(simple_formatter& fmt, log_clock::time_point tp, level lvl) {
    memory_buf_t buf;
    fmt.format_header(tp, lvl, buf);
    return std::string(buf.data(), buf.size());
}

TEST_CASE("format_header produces a fixed-shape header with logger name") {
    simple_formatter fmt{"myname"};
    auto out = format_one(fmt, log_clock::now(), level::info);

    // Layout (byte offsets): the timestamp separators are fixed, then a space at 25.
    //   [ Y Y Y Y - M M - D D   H H : M M : S S . m m m ]
    //   0 1       5     8       11    14    17    20    24 25
    CHECK(out.front() == '[');
    CHECK(out[5] == '-');
    CHECK(out[8] == '-');
    CHECK(out[11] == ' ');
    CHECK(out[14] == ':');
    CHECK(out[17] == ':');
    CHECK(out[20] == '.');
    CHECK(out[24] == ']');
    CHECK(out[25] == ' ');
    CHECK(out.find("[myname]") != std::string::npos);
    CHECK(out.find("[INF] ") != std::string::npos);
    CHECK(out.back() == ' ');  // header always ends with " " (payload appended after)
}

TEST_CASE("format_header omits the name bracket when name is empty") {
    simple_formatter fmt{};
    auto out = format_one(fmt, log_clock::now(), level::warn);

    // [YYYY-MM-DD HH:MM:SS.mmm] [WRN]<space>
    CHECK(out.size() == 32);  // 26 timestamp + "[WRN] "
    CHECK(out.substr(26) == "[WRN] ");
}

TEST_CASE("format_header patches the level tag per call") {
    simple_formatter fmt{};
    auto tp = log_clock::now();

    auto a = format_one(fmt, tp, level::trace);
    auto b = format_one(fmt, tp, level::critical);
    auto c = format_one(fmt, tp, level::off);

    CHECK(a.substr(27, 3) == "TRC");
    CHECK(b.substr(27, 3) == "CRT");
    CHECK(c.substr(27, 3) == "OFF");
}

TEST_CASE("level_offset points at the level character (used by color sinks)") {
    // anon header: "[YYYY-MM-DD HH:MM:SS.mmm] [" -> offset 27
    CHECK(simple_formatter{}.level_offset() == 27);

    // named "abc" header inserts "[abc] " before the level bracket -> offset 27 + 6 = 33
    CHECK(simple_formatter{"abc"}.level_offset() == 33);
}

TEST_CASE("set_logger_name updates the header") {
    simple_formatter fmt{"old"};
    auto out1 = format_one(fmt, log_clock::now(), level::info);
    CHECK(out1.find("[old]") != std::string::npos);

    fmt.set_logger_name("new");
    auto out2 = format_one(fmt, log_clock::now(), level::info);
    CHECK(out2.find("[old]") == std::string::npos);
    CHECK(out2.find("[new]") != std::string::npos);
}

TEST_CASE("format_header patches only the millis when within the same second") {
    // Three timestamps in the same second should share the date+time portion and
    // differ only in the millis triplet at offsets 21..23. Exercises the cached-header
    // path that skips localtime() on sub-second updates.
    simple_formatter fmt{};
    auto base = log_clock::time_point{seconds{1700000000}};
    auto a = format_one(fmt, base, level::info);
    auto b = format_one(fmt, base + milliseconds{123}, level::info);
    auto c = format_one(fmt, base + milliseconds{999}, level::info);

    CHECK(a.substr(21, 3) == "000");
    CHECK(b.substr(21, 3) == "123");
    CHECK(c.substr(21, 3) == "999");

    CHECK(a.substr(1, 19) == b.substr(1, 19));
    CHECK(a.substr(1, 19) == c.substr(1, 19));
}
