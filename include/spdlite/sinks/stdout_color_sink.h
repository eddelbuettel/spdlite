// SPDX-License-Identifier: MIT
// Copyright (c) 2026, Gabi Melman

#pragma once

#include <array>
#include <cstdio>
#include <mutex>
#include <string_view>

#ifdef _WIN32
    #ifndef NOMINMAX
        #define NOMINMAX
    #endif
    #include <windows.h>
#endif

#include "../common.h"
#include "../formatter.h"

namespace spdlite::sinks {

namespace detail {
#ifdef _WIN32
inline void enable_ansi_colors() {
    static std::once_flag flag;
    std::call_once(flag, [] {
        auto* handle = ::GetStdHandle(STD_OUTPUT_HANDLE);
        if (handle != INVALID_HANDLE_VALUE) {
            DWORD mode = 0;
            if (::GetConsoleMode(handle, &mode)) {
                ::SetConsoleMode(handle, mode | ENABLE_VIRTUAL_TERMINAL_PROCESSING);
            }
        }
        handle = ::GetStdHandle(STD_ERROR_HANDLE);
        if (handle != INVALID_HANDLE_VALUE) {
            DWORD mode = 0;
            if (::GetConsoleMode(handle, &mode)) {
                ::SetConsoleMode(handle, mode | ENABLE_VIRTUAL_TERMINAL_PROCESSING);
            }
        }
    });
}
#else
inline void enable_ansi_colors() {}
#endif
}  // namespace detail

namespace ansi_color {
constexpr std::string_view reset = "\033[m";
constexpr std::string_view white = "\033[37m";
constexpr std::string_view cyan = "\033[36m";
constexpr std::string_view green = "\033[32m";
constexpr std::string_view yellow_bold = "\033[33m\033[1m";
constexpr std::string_view red_bold = "\033[31m\033[1m";
constexpr std::string_view bold_on_red = "\033[1m\033[41m";
}  // namespace ansi_color

// wraps ANSI escape codes around the 3-char level tag in the formatted output.
// rebuilds the line into cbuf_ with: [prefix][color][LVL][reset][rest].
struct color_sink {
    explicit color_sink(std::FILE* file = stdout)
        : file_(file) {
        detail::enable_ansi_colors();
        colors_[static_cast<std::size_t>(level::trace)] = ansi_color::white;
        colors_[static_cast<std::size_t>(level::debug)] = ansi_color::cyan;
        colors_[static_cast<std::size_t>(level::info)] = ansi_color::green;
        colors_[static_cast<std::size_t>(level::warn)] = ansi_color::yellow_bold;
        colors_[static_cast<std::size_t>(level::err)] = ansi_color::red_bold;
        colors_[static_cast<std::size_t>(level::critical)] = ansi_color::bold_on_red;
        colors_[static_cast<std::size_t>(level::off)] = ansi_color::reset;
    }

    void write(const char* data, std::size_t size, const log_msg& msg) {
        auto color = colors_[static_cast<std::size_t>(msg.log_level)];
        constexpr std::size_t level_size = 3;
        // with name: "[timestamp] [name] [LVL] " → 27 + name + 3
        // no name:   "[timestamp] [LVL] "        → 27
        auto level_start = msg.logger_name.empty() ? 27 : 27 + msg.logger_name.size() + 3;
        auto level_end = level_start + level_size;

        cbuf_.clear();
        //cbuf_.append(data, data + level_start);
        cbuf_.append(color.data(), color.data() + color.size());
        cbuf_.append(data + level_start, data + level_end);
        cbuf_.append(ansi_color::reset.data(), ansi_color::reset.data() + ansi_color::reset.size());
        cbuf_.append(data + level_end, data + size);
        fwrite_bytes(cbuf_.data(), cbuf_.size(), file_);
    }

    void flush() { std::fflush(file_); }

    void set_color(level lvl, std::string_view color) { colors_[static_cast<std::size_t>(lvl)] = color; }

private:
    std::FILE* file_;
    memory_buf_t cbuf_;
    std::array<std::string_view, levels_count> colors_{};
};

using stdout_color_sink = color_sink;

struct stderr_color_sink : color_sink {
    stderr_color_sink()
        : color_sink(stderr) {}
};

}  // namespace spdlite::sinks
