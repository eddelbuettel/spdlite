// SPDX-License-Identifier: MIT
// Copyright (c) 2026, Gabi Melman

#pragma once

#include "../common.h"

namespace spdlite {

// discards all output - useful for benchmarking the format path without I/O
struct null_sink {
    void write(const log_msg &) {}
    void flush() {}
};

}  // namespace spdlite
