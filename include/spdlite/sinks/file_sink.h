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

// fopen/fwrite with RAII. Creates parent directories automatically.
// Uses _wfopen on Windows for proper Unicode path support.
struct file_sink {
    explicit file_sink(const std::filesystem::path &filename, bool truncate = false) {
        if (auto parent = filename.parent_path(); !parent.empty()) {
            std::filesystem::create_directories(parent);
        }

#ifdef _WIN32
        file_.reset(_wfopen(filename.c_str(), truncate ? L"wb" : L"ab"));
#else
        file_.reset(std::fopen(filename.c_str(), truncate ? "wb" : "ab"));
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
