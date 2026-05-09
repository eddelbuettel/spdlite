// SPDX-License-Identifier: MIT
// Copyright (c) 2026, Gabi Melman

#pragma once

#include <cstdio>
#include <filesystem>
#include <memory>
#include <stdexcept>
#include <string>

#include "../common.h"

namespace spdlite {

// How file_sink opens an existing file: keep its contents (append) or
// discard them (truncate). Default is append.
enum class open_mode { append, truncate };

// fopen/fwrite with RAII. Creates parent directories automatically.
// Uses _wfopen on Windows for proper Unicode path support.
struct file_sink {
    explicit file_sink(const std::filesystem::path &filename, open_mode mode = open_mode::append) {
        if (auto parent = filename.parent_path(); !parent.empty()) {
            std::filesystem::create_directories(parent);
        }

        const bool trunc = (mode == open_mode::truncate);
#ifdef _WIN32
        file_.reset(_wfopen(filename.c_str(), trunc ? L"wb" : L"ab"));
#else
        file_.reset(std::fopen(filename.c_str(), trunc ? "wb" : "ab"));
#endif
        if (!file_) {
            throw std::runtime_error("spdlite: failed to open file: " + filename.string());
        }
    }

    void write(const log_msg &msg) { fwrite_bytes(msg.formatted.data(), msg.formatted.size(), file_.get()); }
    void flush() { std::fflush(file_.get()); }

private:
    std::unique_ptr<std::FILE, detail::file_closer> file_;
};

}  // namespace spdlite
