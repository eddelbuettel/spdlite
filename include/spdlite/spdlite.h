// SPDX-License-Identifier: MIT
// Copyright (c) 2026, Gabi Melman

#pragma once

// Public include. Pulls in the minimal types from common.h, the formatter,
// and adds the logger template + the format-string aliases users see.
// Sinks include common.h directly - they don't need this file.

#include <atomic>
#include <cstdio>
#include <iterator>
#include <mutex>
#include <string>
#include <tuple>
#include <utility>

#include "common.h"
#include "formatter.h"

namespace spdlite {

#ifdef SPDLITE_USE_STD_FORMAT
template <typename... Args>
using format_string_t = std::format_string<Args...>;
using format_args_t = std::format_args;
using format_string_view_t = std::string_view;
#else
template <typename... Args>
using format_string_t = fmt::format_string<Args...>;
using format_args_t = fmt::format_args;
using format_string_view_t = fmt::string_view;
#endif

using atomic_level_t = std::atomic<level>;

// no-op mutex for logger_st
struct null_mutex {
    void lock() noexcept {}
    void unlock() noexcept {}
};

// Logger class. Formats log messages and forwards them to the sinks.
template <typename Mutex, typename... Sinks>
class logger {
public:
    explicit logger(std::string name, Sinks... sinks)
        : name_(std::move(name)),
          formatter_(name_),
          sinks_(std::move(sinks)...) {}

    logger() = default;

    logger(logger&& other) noexcept
        : name_(std::move(other.name_)),
          level_(other.level_.load(std::memory_order_relaxed)),
          flush_level_(other.flush_level_.load(std::memory_order_relaxed)),
          formatter_(std::move(other.formatter_)),
          buf_(std::move(other.buf_)),
          sinks_(std::move(other.sinks_)) {
        other.level_.store(level::off, std::memory_order_relaxed);
    }
   
    logger& operator=(logger&&) = delete;
    logger(const logger&) = delete;
    logger& operator=(const logger&) = delete;

    template <typename... Args>
    void log(level lvl, format_string_t<Args...> fmt, Args&&... args) const noexcept {
        if (should_log(lvl)) dispatch_fmt_(lvl, fmt, std::forward<Args>(args)...);
    }

    void log(level lvl, string_view_t msg) const noexcept {
        if (should_log(lvl)) log_sv_(lvl, msg);
    }

    // Per-level convenience overloads - forward to log() with a compile-time level.
    template <typename... Args>
    void trace(format_string_t<Args...> fmt, Args&&... args) const noexcept {
        log(level::trace, fmt, std::forward<Args>(args)...);
    }
    template <typename... Args>
    void debug(format_string_t<Args...> fmt, Args&&... args) const noexcept {
        log(level::debug, fmt, std::forward<Args>(args)...);
    }
    template <typename... Args>
    void info(format_string_t<Args...> fmt, Args&&... args) const noexcept {
        log(level::info, fmt, std::forward<Args>(args)...);
    }
    template <typename... Args>
    void warn(format_string_t<Args...> fmt, Args&&... args) const noexcept {
        log(level::warn, fmt, std::forward<Args>(args)...);
    }
    template <typename... Args>
    void error(format_string_t<Args...> fmt, Args&&... args) const noexcept {
        log(level::err, fmt, std::forward<Args>(args)...);
    }
    template <typename... Args>
    void critical(format_string_t<Args...> fmt, Args&&... args) const noexcept {
        log(level::critical, fmt, std::forward<Args>(args)...);
    }

    // string_view overloads - no formatting, just header + payload
    void trace(string_view_t msg) const noexcept { log(level::trace, msg); }
    void debug(string_view_t msg) const noexcept { log(level::debug, msg); }
    void info(string_view_t msg) const noexcept { log(level::info, msg); }
    void warn(string_view_t msg) const noexcept { log(level::warn, msg); }
    void error(string_view_t msg) const noexcept { log(level::err, msg); }
    void critical(string_view_t msg) const noexcept { log(level::critical, msg); }

    [[nodiscard]] bool should_log(level msg_level) const noexcept { return msg_level >= level_.load(std::memory_order_relaxed); }
    [[nodiscard]] bool should_flush(level msg_level) const noexcept {
        return msg_level >= flush_level_.load(std::memory_order_relaxed);
    }

    void log_level(level lvl) noexcept { level_.store(lvl, std::memory_order_relaxed); }
    [[nodiscard]] level log_level() const noexcept { return level_.load(std::memory_order_relaxed); }
    void flush_level(level lvl) noexcept { flush_level_.store(lvl, std::memory_order_relaxed); }
    [[nodiscard]] level flush_level() const noexcept { return flush_level_.load(std::memory_order_relaxed); }
    [[nodiscard]] const std::string& name() const noexcept { return name_; }

    void flush() const noexcept {
        std::lock_guard<Mutex> lock(mutex_);
        std::apply([](auto&... s) { (s.flush(), ...); }, sinks_);
    }

private:
    std::string name_;
    atomic_level_t level_{level::info};
    atomic_level_t flush_level_{level::off};  // off => never auto-flush
    mutable Mutex mutex_;
    mutable simple_formatter formatter_;
    mutable memory_buf_t buf_;
    mutable std::tuple<Sinks...> sinks_;

    // string_view path - no formatting needed, just header + raw payload
    void log_sv_(level lvl, string_view_t sv) const noexcept {
        const auto now = log_clock::now();  // timestamp before lock for accuracy
        std::lock_guard<Mutex> lock(mutex_);
        buf_.clear();
        formatter_.format_header(now, lvl, buf_);
        const auto payload_start = buf_.size();
        buf_.append(sv.data(), sv.data() + sv.size());
        const auto payload_end = buf_.size();
#ifdef _WIN32
        buf_.push_back('\r');
#endif
        buf_.push_back('\n');
        string_view_t formatted{buf_.data(), buf_.size()};
        string_view_t payload{buf_.data() + payload_start, payload_end - payload_start};
        log_msg msg(now, name_, lvl, formatted, payload);
        std::apply([&](auto&... s) { (s.write(msg), ...); }, sinks_);
        if (should_flush(lvl)) std::apply([](auto&... s) { (s.flush(), ...); }, sinks_);
    }

    // per-Args trampoline - type-erases args, forwards to log_fmt_args_.
    template <typename... Args>
    void dispatch_fmt_(level lvl, format_string_t<Args...> fmt_str, Args&&... args) const noexcept {
#ifdef SPDLITE_USE_STD_FORMAT
        log_fmt_args_(lvl, fmt_str.get(), std::make_format_args(args...));
#else
        log_fmt_args_(lvl, fmt_str, fmt::make_format_args(args...));
#endif
    }

    // format and send the message to sinks.
    // All formatting + dispatch happens under a single lock, so buf_ is shared state.
    void log_fmt_args_(level lvl, format_string_view_t fmt_str, format_args_t args) const noexcept {
        try {
            const auto now = log_clock::now();  // timestamp before lock for accuracy
            std::lock_guard<Mutex> lock(mutex_);
            buf_.clear();
            formatter_.format_header(now, lvl, buf_);
            const auto payload_start = buf_.size();
#ifdef SPDLITE_USE_STD_FORMAT
            std::vformat_to(std::back_inserter(buf_), fmt_str, args);
#else
            fmt::vformat_to(fmt::appender(buf_), fmt_str, args);
#endif
            const auto payload_end = buf_.size();
#ifdef _WIN32
            buf_.push_back('\r');
#endif
            buf_.push_back('\n');
            string_view_t formatted{buf_.data(), buf_.size()};
            string_view_t payload{buf_.data() + payload_start, payload_end - payload_start};
            log_msg msg(now, name_, lvl, formatted, payload);
            std::apply([&](auto&... s) { (s.write(msg), ...); }, sinks_);
            if (should_flush(lvl)) std::apply([](auto&... s) { (s.flush(), ...); }, sinks_);
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
