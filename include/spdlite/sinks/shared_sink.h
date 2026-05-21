// SPDX-License-Identifier: MIT
// Copyright (c) 2026, Gabi Melman

#pragma once

#include <memory>
#include <mutex>

#include "../common.h"

namespace spdlite {

// Wraps any sink so multiple loggers can write through one underlying instance.
// Copies share the sink and the lock; the lock serializes cross-logger writes
// (each logger's own mutex still serializes within one logger).
template <typename Sink>
class shared_sink {
public:
    explicit shared_sink(std::shared_ptr<Sink> sink)
        : sink_(std::move(sink)),
          mutex_(std::make_shared<std::mutex>()) {}

    void write(const log_msg& msg) {
        std::lock_guard<std::mutex> lock(*mutex_);
        sink_->write(msg);
    }
    void flush() {
        std::lock_guard<std::mutex> lock(*mutex_);
        sink_->flush();
    }

    [[nodiscard]] const std::shared_ptr<Sink>& get() const noexcept { return sink_; }

private:
    std::shared_ptr<Sink> sink_;
    std::shared_ptr<std::mutex> mutex_;
};

}  // namespace spdlite
