// SPDX-License-Identifier: MIT
// Copyright (c) 2026, Gabi Melman

#pragma once

// Minimal types shared by logger.h and the sinks. Designed so a sink header
// can include just this file rather than the whole logger template.

#define SPDLITE_VER_MAJOR 0
#define SPDLITE_VER_MINOR 1
#define SPDLITE_VER_PATCH 0

#include <array>
#include <atomic>
#include <cassert>
#include <chrono>
#include <cstdint>
#include <cstdio>
#include <cstring>
#include <string>
#include <string_view>

#ifdef SPDLITE_USE_STD_FORMAT
    #include <format>
#else
    #ifndef FMT_HEADER_ONLY
        #define FMT_HEADER_ONLY
    #endif
    #include "fmt/base.h"
    #include "fmt/format.h"
#endif

namespace spdlite {

using log_clock = std::chrono::system_clock;
using string_view_t = std::string_view;

#ifdef SPDLITE_USE_STD_FORMAT

// Stack-allocated buffer with heap fallback. Used as memory_buf_t when
// SPDLITE_USE_STD_FORMAT is defined (replacing fmt::basic_memory_buffer).
// Typical log lines are ~120 bytes, so 250 bytes covers most without heap allocation.
template <std::size_t StackSize = 250>
class stack_buf {
public:
    using value_type = char;  // required for std::back_inserter
    stack_buf() = default;
    stack_buf(const stack_buf&) = delete;
    stack_buf& operator=(const stack_buf&) = delete;
    stack_buf(stack_buf&& other) noexcept
        : heap_storage_(std::move(other.heap_storage_)) {
        if (other.heap_) {
            heap_ = &heap_storage_;
        } else {
            size_ = other.size_;
            std::memcpy(stack_, other.stack_, size_);
        }
        other.size_ = 0;
        other.heap_ = nullptr;
    }
    stack_buf& operator=(stack_buf&&) = delete;

    void push_back(char c) {
        // already on heap
        if (heap_) {
            heap_->push_back(c);
            return;
        }

        // fits in stack
        if (size_ < StackSize) {
            stack_[size_++] = c;
            return;
        }

        // stack full - spill to heap
        spill_to_heap();
        heap_->push_back(c);
    }

    void append(const char* begin, const char* end) {
        auto n = static_cast<std::size_t>(end - begin);

        // already on heap - append directly
        if (heap_) {
            heap_->append(begin, end);
            return;
        }

        // fits in stack - memcpy
        if (size_ + n <= StackSize) {
            std::memcpy(stack_ + size_, begin, n);
            size_ += n;
            return;
        }

        // doesn't fit - move stack content to heap, then append
        spill_to_heap();
        heap_->append(begin, end);
    }

    void clear() noexcept {
        heap_ = nullptr;
        size_ = 0;
    }

    [[nodiscard]] const char* data() const noexcept { return heap_ ? heap_->data() : stack_; }
    [[nodiscard]] char* data() noexcept { return heap_ ? heap_->data() : stack_; }
    [[nodiscard]] std::size_t size() const noexcept { return heap_ ? heap_->size() : size_; }

private:
    char stack_[StackSize];
    std::size_t size_{0};
    std::string* heap_{nullptr};
    std::string heap_storage_;

    void spill_to_heap() {
        heap_storage_.assign(stack_, size_);
        heap_ = &heap_storage_;
    }
};

using memory_buf_t = stack_buf<250>;

template <typename... Args>
using format_string_t = std::format_string<Args...>;
using format_args_t = std::format_args;
using format_string_view_t = std::string_view;

#else
// when using fmt, reuse its buffer directly - same stack/heap design, no wrapper needed
using memory_buf_t = fmt::basic_memory_buffer<char, 250>;

template <typename... Args>
using format_string_t = fmt::format_string<Args...>;
using format_args_t = fmt::format_args;
using format_string_view_t = fmt::string_view;
#endif

enum class level : std::uint8_t { trace = 0, debug = 1, info = 2, warn = 3, err = 4, critical = 5, off = 6, n_levels = 7 };

constexpr auto levels_count = static_cast<std::size_t>(level::n_levels);

// fixed single-char tags - the formatter patches one byte directly into the cached header
constexpr std::array<char, levels_count> level_names{'T', 'D', 'I', 'W', 'E', 'C', 'O'};

[[nodiscard]] constexpr char to_char(level lvl) noexcept {
    assert(static_cast<std::size_t>(lvl) < levels_count);
    return level_names[static_cast<std::size_t>(lvl)];
}

namespace detail {
// shared FILE* deleter for unique_ptr in file-backed sinks
struct file_closer {
    void operator()(std::FILE *f) const noexcept {
        if (f) std::fclose(f);
    }
};
}  // namespace detail

// non-locking fwrite - the logger already holds its own mutex, so the per-call stdio lock is redundant
inline bool fwrite_bytes(const void *ptr, std::size_t n, std::FILE *fp) {
#if defined(_WIN32)
    return ::_fwrite_nolock(ptr, 1, n, fp) == n;
#elif defined(__linux__)
    return ::fwrite_unlocked(ptr, 1, n, fp) == n;
#else
    return std::fwrite(ptr, 1, n, fp) == n;
#endif
}

// lightweight message descriptor passed to sinks - all views, no ownership.
// `formatted` covers the whole line the logger produced (header + payload + newline).
// `payload` is the raw user message, no header, no trailing newline.
struct log_msg {
    log_clock::time_point time;
    string_view_t logger_name;
    level log_level{level::off};
    string_view_t formatted;
    string_view_t payload;

    log_msg() = default;

    log_msg(log_clock::time_point log_time, string_view_t name, level lvl,
            string_view_t line, string_view_t raw)
        : time(log_time),
          logger_name(name),
          log_level(lvl),
          formatted(line),
          payload(raw) {}
};

// logger-side helpers used by the logger template
using atomic_level_t = std::atomic<level>;

// no-op mutex for logger_st
struct null_mutex {
    void lock() noexcept {}
    void unlock() noexcept {}
};

}  // namespace spdlite
