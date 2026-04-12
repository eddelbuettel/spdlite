// SPDX-License-Identifier: MIT
// Copyright (c) 2026, Gabi Melman

#pragma once

#include <cstdio>
#include <iterator>
#include <mutex>
#include <string>
#include <tuple>
#include <utility>

#include "common.h"
#include "formatter.h"

namespace spdlite {

// Variadic-template logger. Sinks are compile-time parameters — no virtual dispatch.
// Mutex selects thread safety: std::mutex for multi-threaded, null_mutex for single-threaded.
// All formatting and sink dispatch happens under a single lock (buf_ is shared state).
template <typename Mutex, typename... Sinks>
class logger {
public:
    explicit logger(std::string name, Sinks... sinks)
        : name_(std::move(name)),
          formatter_(name_),
          sinks_(std::move(sinks)...) {}

    // default-construct sinks (e.g. stdout_color_sink)
    explicit logger(std::string name) requires(sizeof...(Sinks) > 0)
        : name_(std::move(name)),
          formatter_(name_) {}

    // nameless logger — output omits [name] field
    explicit logger(Sinks... sinks) requires(sizeof...(Sinks) > 0)
        : sinks_(std::move(sinks)...) {}

    logger() = default;

    template <typename... Args>
    void trace(format_string_t<Args...> fmt, Args&&... args) noexcept {
        if (should_log(level::trace)) log_fmt_(level::trace, fmt, std::forward<Args>(args)...);
    }

    template <typename... Args>
    void debug(format_string_t<Args...> fmt, Args&&... args) noexcept {
        if (should_log(level::debug)) log_fmt_(level::debug, fmt, std::forward<Args>(args)...);
    }

    template <typename... Args>
    void info(format_string_t<Args...> fmt, Args&&... args) noexcept {
        if (should_log(level::info)) log_fmt_(level::info, fmt, std::forward<Args>(args)...);
    }

    template <typename... Args>
    void warn(format_string_t<Args...> fmt, Args&&... args) noexcept {
        if (should_log(level::warn)) log_fmt_(level::warn, fmt, std::forward<Args>(args)...);
    }

    template <typename... Args>
    void error(format_string_t<Args...> fmt, Args&&... args) noexcept {
        if (should_log(level::err)) log_fmt_(level::err, fmt, std::forward<Args>(args)...);
    }

    template <typename... Args>
    void critical(format_string_t<Args...> fmt, Args&&... args) noexcept {
        if (should_log(level::critical)) log_fmt_(level::critical, fmt, std::forward<Args>(args)...);
    }

    // string_view overloads — no formatting, just header + payload
    void trace(string_view_t msg) noexcept { if (should_log(level::trace)) log_sv_(level::trace, msg); }
    void debug(string_view_t msg) noexcept { if (should_log(level::debug)) log_sv_(level::debug, msg); }
    void info(string_view_t msg) noexcept { if (should_log(level::info)) log_sv_(level::info, msg); }
    void warn(string_view_t msg) noexcept { if (should_log(level::warn)) log_sv_(level::warn, msg); }
    void error(string_view_t msg) noexcept { if (should_log(level::err)) log_sv_(level::err, msg); }
    void critical(string_view_t msg) noexcept { if (should_log(level::critical)) log_sv_(level::critical, msg); }

    [[nodiscard]] bool should_log(level msg_level) const noexcept { return msg_level >= level_.load(std::memory_order_relaxed); }

    void set_level(level lvl) noexcept { level_.store(lvl, std::memory_order_relaxed); }
    [[nodiscard]] level log_level() const noexcept { return level_.load(std::memory_order_relaxed); }
    [[nodiscard]] const std::string& name() const noexcept { return name_; }

    void flush() noexcept {
        std::lock_guard<Mutex> lock(mutex_);
        std::apply([](auto&... s) { (s.flush(), ...); }, sinks_);
    }

private:
    std::string name_;
    atomic_level_t level_{level::info};
    Mutex mutex_;
    simple_formatter formatter_;
    memory_buf_t buf_;
    std::tuple<Sinks...> sinks_;

    // string_view path — no formatting needed, just header + raw payload
    void log_sv_(level lvl, string_view_t sv) noexcept {
        auto now = log_clock::now();  // timestamp before lock for accuracy
        std::lock_guard<Mutex> lock(mutex_);
        buf_.clear();
        formatter_.format_header(now, lvl, buf_);
        buf_.append(sv.data(), sv.data() + sv.size());
#ifdef _WIN32
        buf_.push_back('\r');
#endif
        buf_.push_back('\n');
        log_msg msg(now, name_, lvl, sv);
        std::apply([&](auto&... s) { (s.write(buf_.data(), buf_.size(), msg), ...); }, sinks_);
    }

    // fmt/std::format path — format payload into buf_ after header
    template <typename... Args>
    void log_fmt_(level lvl, format_string_t<Args...> fmt_str, Args&&... args) noexcept {
        try {
            auto now = log_clock::now();  // timestamp before lock for accuracy
            std::lock_guard<Mutex> lock(mutex_);
            buf_.clear();
            formatter_.format_header(now, lvl, buf_);
#ifdef SPDLITE_USE_STD_FORMAT
            std::vformat_to(std::back_inserter(buf_), fmt_str.get(), std::make_format_args(args...));
#else
            fmt::vformat_to(fmt::appender(buf_), fmt_str, fmt::make_format_args(args...));
#endif
    #ifdef _WIN32
        buf_.push_back('\r');
#endif
        buf_.push_back('\n');
            log_msg msg(now, name_, lvl, {});
            std::apply([&](auto&... s) { (s.write(buf_.data(), buf_.size(), msg), ...); }, sinks_);
        } catch (const std::exception& ex) {
            std::fprintf(stderr, "spdlite: log error: %s\n", ex.what());
        } catch (...) {
            std::fprintf(stderr, "spdlite: unknown log error\n");
        }
    }
};

// logger_mt: thread-safe (std::mutex). Serializes format + dispatch per log call.
template <typename... Sinks>
using logger_mt = logger<std::mutex, Sinks...>;

// logger_st: single-threaded (null_mutex). Zero locking overhead.
template <typename... Sinks>
using logger_st = logger<null_mutex, Sinks...>;

}  // namespace spdlite
