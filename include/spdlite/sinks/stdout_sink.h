// SPDX-License-Identifier: MIT
// Copyright (c) 2026, Gabi Melman

#pragma once

#include <cstdio>

#include "../common.h"

namespace spdlite {

// plain fwrite to stdout/stderr - no colors, no buffering tricks
struct stdout_sink {
    explicit stdout_sink(std::FILE* file = stdout)
        : file_(file) {}
    void write(const log_msg& msg) { detail::fwrite_bytes(msg.formatted.data(), msg.formatted.size(), file_); }
    void flush() { std::fflush(file_); }

private:
    std::FILE* file_;
};

struct stderr_sink : stdout_sink {
    stderr_sink()
        : stdout_sink(stderr) {}
};

}  // namespace spdlite
