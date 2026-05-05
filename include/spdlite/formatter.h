// SPDX-License-Identifier: MIT
// Copyright (c) 2026, Gabi Melman

#pragma once

#include <chrono>
#include <ctime>
#include <string>

#include "common.h"

namespace spdlite {

inline void put2(char* dst, int n) {
    dst[0] = static_cast<char>('0' + n / 10);
    dst[1] = static_cast<char>('0' + n % 10);
}

inline void put3(char* dst, int n) {
    dst[0] = static_cast<char>('0' + n / 100);
    dst[1] = static_cast<char>('0' + (n / 10) % 10);
    dst[2] = static_cast<char>('0' + n % 10);
}

inline void put4(char* dst, int n) {
    dst[0] = static_cast<char>('0' + n / 1000);
    dst[1] = static_cast<char>('0' + (n / 100) % 10);
    dst[2] = static_cast<char>('0' + (n / 10) % 10);
    dst[3] = static_cast<char>('0' + n % 10);
}

// Fixed format: [YYYY-MM-DD HH:MM:SS.mmm] [name] [L] payload\n
// When name is empty: [YYYY-MM-DD HH:MM:SS.mmm] [L] payload\n
//
// Caches the entire header as a single string.
// Per-call: patch 3 millis bytes + 1 level byte, one memcpy for header, append payload + '\n'.
struct simple_formatter {
    explicit simple_formatter(string_view_t logger_name = {}) { rebuild_header(logger_name); }

    void set_logger_name(string_view_t name) { rebuild_header(name); }

    // append "[YYYY-MM-DD HH:MM:SS.mmm] [name] [L] " to dest.
    // only the millis (3 bytes) and level (1 byte) are patched per call.
    // the full timestamp (date + h:m:s) is rebuilt only when the second changes.
    void format_header(log_clock::time_point time, level lvl, memory_buf_t& dest) {
        using namespace std::chrono;

        auto time_since_epoch = time.time_since_epoch();
        auto secs = duration_cast<seconds>(time_since_epoch);
        auto millis = duration_cast<milliseconds>(time_since_epoch) - duration_cast<milliseconds>(secs);

        // rebuild date/time portion only on second boundary change
        if (secs != last_secs_) {
            last_secs_ = secs;
            rebuild_timestamp(static_cast<std::time_t>(secs.count()));
        }

        // patch millis and level in the cached header - 4 byte writes total
        put3(header_.data() + millis_offset_, static_cast<int>(millis.count()));
        header_[level_offset_] = to_char(lvl);

        dest.append(header_.data(), header_.data() + header_.size());
    }

    // Offset where the level starts in formatted output (for color sinks)
    [[nodiscard]] std::size_t level_offset() const noexcept { return level_offset_; }

private:
    static constexpr std::size_t millis_offset_ = 21;
    // header: "[YYYY-MM-DD HH:MM:SS.mmm] [name] [L] "
    std::string header_;
    std::size_t level_offset_{};
    std::chrono::seconds last_secs_{};

    void rebuild_header(string_view_t logger_name) {
        header_.clear();
        header_ = "[0000-00-00 00:00:00.000] ";  // 26 chars
        if (!logger_name.empty()) {
            header_.push_back('[');
            header_.append(logger_name);
            header_.append("] ");
        }
        header_.push_back('[');
        level_offset_ = header_.size();
        header_.append("I] ");  // placeholder level (1 char)
        last_secs_ = {};        // force timestamp rebuild on next format
    }

    void rebuild_timestamp(std::time_t time_t_val) {
        std::tm tm{};
#ifdef _WIN32
        localtime_s(&tm, &time_t_val);
#else
        localtime_r(&time_t_val, &tm);
#endif
        put4(header_.data() + 1, tm.tm_year + 1900);
        header_[5] = '-';
        put2(header_.data() + 6, tm.tm_mon + 1);
        header_[8] = '-';
        put2(header_.data() + 9, tm.tm_mday);
        header_[11] = ' ';
        put2(header_.data() + 12, tm.tm_hour);
        header_[14] = ':';
        put2(header_.data() + 15, tm.tm_min);
        header_[17] = ':';
        put2(header_.data() + 18, tm.tm_sec);
        header_[20] = '.';
        header_[24] = ']';
        header_[25] = ' ';
        header_[26] = '[';
    }
};

}  // namespace spdlite
