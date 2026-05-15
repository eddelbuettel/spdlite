// SPDX-License-Identifier: MIT
// In-memory sink used by tests. Records every write and counts flushes.
//
// State lives behind a shared_ptr so copies of the sink (the logger stores
// sinks by value in a tuple) observe the same state.

#pragma once

#include <memory>
#include <mutex>
#include <string>
#include <vector>

#include "spdlite/common.h"

namespace spdlite_test {

struct capture_sink {
    struct state_t {
        std::mutex mu;
        std::vector<std::string> formatted;
        std::vector<std::string> payloads;
        std::vector<spdlite::level> levels;
        std::size_t flush_count{0};
    };

    capture_sink()
        : state(std::make_shared<state_t>()) {}

    void write(const spdlite::log_msg& msg) {
        std::lock_guard<std::mutex> lock(state->mu);
        state->formatted.emplace_back(msg.formatted);
        state->payloads.emplace_back(msg.payload);
        state->levels.push_back(msg.log_level);
    }

    void flush() {
        std::lock_guard<std::mutex> lock(state->mu);
        ++state->flush_count;
    }

    std::shared_ptr<state_t> state;
};

}  // namespace spdlite_test
