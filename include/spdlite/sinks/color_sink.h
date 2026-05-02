// SPDX-License-Identifier: MIT
// Copyright (c) 2026, Gabi Melman

#pragma once

#include <array>

#include "../common.h"

#ifdef _WIN32

// ==================== Windows: native console API ====================

#ifndef NOMINMAX
#define NOMINMAX
#endif
#include <windows.h>

namespace spdlite::sinks {

namespace detail {

// uses SetConsoleTextAttribute + WriteConsoleA for colored output.
// falls back to WriteFile when output is redirected to a file.
struct color_sink_base {
    explicit color_sink_base(HANDLE handle)
        : handle_(handle) {
        // detect if we're writing to a real console
        DWORD console_mode = 0;
        is_console_ = ::GetConsoleMode(handle_, &console_mode) != 0;

        // save original attributes for reset
        CONSOLE_SCREEN_BUFFER_INFO info{};
        if (is_console_ && ::GetConsoleScreenBufferInfo(handle_, &info)) {
            orig_attribs_ = info.wAttributes;
        }

        colors_[static_cast<std::size_t>(level::trace)] = FOREGROUND_RED | FOREGROUND_GREEN | FOREGROUND_BLUE;
        colors_[static_cast<std::size_t>(level::debug)] = FOREGROUND_GREEN | FOREGROUND_BLUE;
        colors_[static_cast<std::size_t>(level::info)] = FOREGROUND_GREEN;
        colors_[static_cast<std::size_t>(level::warn)] = FOREGROUND_RED | FOREGROUND_GREEN | FOREGROUND_INTENSITY;
        colors_[static_cast<std::size_t>(level::err)] = FOREGROUND_RED | FOREGROUND_INTENSITY;
        colors_[static_cast<std::size_t>(level::critical)] =
            BACKGROUND_RED | FOREGROUND_RED | FOREGROUND_GREEN | FOREGROUND_BLUE | FOREGROUND_INTENSITY;
        colors_[static_cast<std::size_t>(level::off)] = 0;
    }

    void write(const log_msg &msg) {
        if (handle_ == nullptr || handle_ == INVALID_HANDLE_VALUE) return;

        const char *data = msg.formatted.data();
        auto size = msg.formatted.size();

        if (!is_console_) {
            // redirected to file — write plain text
            DWORD written = 0;
            ::WriteFile(handle_, data, static_cast<DWORD>(size), &written, nullptr);
            return;
        }

        constexpr std::size_t level_size = 1;
        auto level_start = msg.logger_name.empty() ? 27 : 27 + msg.logger_name.size() + 3;
        auto level_end = level_start + level_size;

        // before level tag
        write_console_(data, level_start);
        // colored level tag
        ::SetConsoleTextAttribute(handle_, colors_[static_cast<std::size_t>(msg.log_level)]);
        write_console_(data + level_start, level_size);
        // reset and remainder
        ::SetConsoleTextAttribute(handle_, orig_attribs_);
        write_console_(data + level_end, size - level_end);
    }

    void flush() {}  // windows console is unbuffered

    void set_color(level lvl, WORD color) { colors_[static_cast<std::size_t>(lvl)] = color; }

private:
    HANDLE handle_;
    WORD orig_attribs_{FOREGROUND_RED | FOREGROUND_GREEN | FOREGROUND_BLUE};
    bool is_console_{false};
    std::array<WORD, levels_count> colors_{};

    void write_console_(const char *data, std::size_t size) {
        if (size > 0) {
            DWORD written = 0;
            ::WriteConsoleA(handle_, data, static_cast<DWORD>(size), &written, nullptr);
        }
    }
};

}  // namespace detail

struct color_stdout : detail::color_sink_base {
    color_stdout() : color_sink_base(::GetStdHandle(STD_OUTPUT_HANDLE)) {}
};

struct color_stderr : detail::color_sink_base {
    color_stderr() : color_sink_base(::GetStdHandle(STD_ERROR_HANDLE)) {}
};

}  // namespace spdlite::sinks

#else

// ==================== Linux/macOS: ANSI escape codes ====================

#include <cstdio>
#include <string_view>

namespace spdlite::sinks {

namespace ansi_color {
constexpr std::string_view reset = "\033[m";
constexpr std::string_view white = "\033[37m";
constexpr std::string_view cyan = "\033[36m";
constexpr std::string_view green = "\033[32m";
constexpr std::string_view yellow_bold = "\033[33m\033[1m";
constexpr std::string_view red_bold = "\033[31m\033[1m";
constexpr std::string_view bold_on_red = "\033[1m\033[41m";
}  // namespace ansi_color

namespace detail {

// wraps ANSI escape codes around the 1-char level tag in the formatted output.
// rebuilds the line into cbuf_ with: [prefix][color][LVL][reset][rest].
struct color_sink_base {
    explicit color_sink_base(std::FILE *file)
        : file_(file) {
        colors_[static_cast<std::size_t>(level::trace)] = ansi_color::white;
        colors_[static_cast<std::size_t>(level::debug)] = ansi_color::cyan;
        colors_[static_cast<std::size_t>(level::info)] = ansi_color::green;
        colors_[static_cast<std::size_t>(level::warn)] = ansi_color::yellow_bold;
        colors_[static_cast<std::size_t>(level::err)] = ansi_color::red_bold;
        colors_[static_cast<std::size_t>(level::critical)] = ansi_color::bold_on_red;
        colors_[static_cast<std::size_t>(level::off)] = ansi_color::reset;
    }

    void write(const log_msg &msg) {
        const char *data = msg.formatted.data();
        auto size = msg.formatted.size();
        auto color = colors_[static_cast<std::size_t>(msg.log_level)];
        constexpr std::size_t level_size = 1;
        auto level_start = msg.logger_name.empty() ? 27 : 27 + msg.logger_name.size() + 3;
        auto level_end = level_start + level_size;

        cbuf_.clear();
        cbuf_.append(data, data + level_start);
        cbuf_.append(color.data(), color.data() + color.size());
        cbuf_.append(data + level_start, data + level_end);
        cbuf_.append(ansi_color::reset.data(), ansi_color::reset.data() + ansi_color::reset.size());
        cbuf_.append(data + level_end, data + size);
        fwrite_bytes(cbuf_.data(), cbuf_.size(), file_);
    }

    void flush() { std::fflush(file_); }

    void set_color(level lvl, std::string_view color) { colors_[static_cast<std::size_t>(lvl)] = color; }

private:
    std::FILE *file_;
    memory_buf_t cbuf_;
    std::array<std::string_view, levels_count> colors_{};
};

}  // namespace detail

struct color_stdout : detail::color_sink_base {
    color_stdout() : color_sink_base(stdout) {}
};

struct color_stderr : detail::color_sink_base {
    color_stderr() : color_sink_base(stderr) {}
};

}  // namespace spdlite::sinks

#endif
