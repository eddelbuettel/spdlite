// SPDX-License-Identifier: MIT
// Copyright (c) 2026, Gabi Melman

#pragma once

#include <chrono>
#include <cstddef>
#include <cstdio>
#include <filesystem>
#include <memory>
#include <optional>
#include <stdexcept>
#include <string>
#include <system_error>
#include <thread>

#include "../common.h"

namespace spdlite {

// File sink with a max_size cap. Active file is always at base_filename;
// archives use a monotonic counter that resumes across restarts.
// On rotation:
// rename active -> base.<N>.ext, open fresh active, drop the archive that
// fell out of the window. One rename + at most one delete per rotation.
//
// max_files = 0 disables archives (truncate-only).
//
// Example: base="logs/app.txt", max_files=3, after 10 rotations:
//   logs/app.txt   app.10.txt        app.9.txt   app.8.txt
//   (active)       (newest archive)              (oldest kept)
struct rotating_file_sink {
    rotating_file_sink(const std::filesystem::path &base_filename, std::size_t max_size, std::size_t max_files = 1)
        : base_filename_(base_filename),
          max_size_(max_size),
          max_files_(max_files) {
        if (max_size == 0) {
            throw std::invalid_argument("rotating_file_sink: max_size must be > 0");
        }
        if (auto parent = base_filename_.parent_path(); !parent.empty()) {
            std::filesystem::create_directories(parent);
        }

        // resume the counter from the highest existing archive, and clean up
        // any orphans below the [highest - max_files + 1, highest] window
        next_index_ = scan_and_prune_archives_() + 1;

        // pick up existing active size so cross-restart logs are preserved
        std::error_code ec;
        if (auto sz = std::filesystem::file_size(base_filename_, ec); !ec) {
            current_size_ = static_cast<std::size_t>(sz);
        }

        open_(false);
    }

    void write(const log_msg &msg) {
        if (!file_) return;  // degraded state after a failed rotation - drop silently
        const auto bytes = msg.formatted.size();
        auto new_size = current_size_ + bytes;
        if (new_size > max_size_ && current_size_ > 0) {
            std::fflush(file_.get());
            rotate_();
            if (!file_) return;  // rotation fallback failed too
            new_size = bytes;
        }
        if (fwrite_bytes(msg.formatted.data(), bytes, file_.get())) {
            current_size_ = new_size;
        }
    }

    void flush() {
        if (file_) std::fflush(file_.get());
    }

private:
    std::filesystem::path base_filename_;
    std::size_t max_size_;
    std::size_t max_files_;
    std::size_t current_size_{0};
    std::size_t next_index_{1};  // index assigned to the next archive
    std::unique_ptr<std::FILE, detail::file_closer> file_;

    // archive_path_(5) for "logs/app.txt" -> "logs/app.5.txt".
    std::filesystem::path archive_path_(std::size_t n) const {
        auto stem = base_filename_.stem().string();
        auto ext = base_filename_.extension().string();
        auto dir = base_filename_.parent_path();
        auto name = stem + "." + std::to_string(n) + ext;
        return dir.empty() ? std::filesystem::path(name) : dir / name;
    }

    void open_(bool truncate) {
#ifdef _WIN32
        std::FILE *fp = _wfopen(base_filename_.c_str(), truncate ? L"wb" : L"ab");
#else
        std::FILE *fp = std::fopen(base_filename_.c_str(), truncate ? "wb" : "ab");
#endif
        if (!fp) {
            throw std::runtime_error("spdlite: failed to open file: " + base_filename_.string());
        }
        file_.reset(fp);
    }

    // close active, rename active -> archive(next_index_), drop the archive
    // that just rolled out of the window, open fresh active.
    // on rename failure fall back to truncating in place to bound file size.
    void rotate_() {
        file_.reset();  // close
        const auto archive = archive_path_(next_index_);
        if (!try_rename_(base_filename_, archive)) {
            // give up on rotation; just truncate active in place to bound size
            open_(true);
            current_size_ = 0;
            return;
        }
        ++next_index_;
        // delete the archive that just left the window (best-effort)
        if (next_index_ > max_files_ + 1) {
            std::error_code ec;
            std::filesystem::remove(archive_path_(next_index_ - max_files_ - 1), ec);
        }
        open_(false);  // base no longer exists, "ab" creates fresh
        current_size_ = 0;
    }

    // rename src->dst with retries after a short sleep. workaround for the
    // windows issue where antivirus / indexer can briefly hold the file and
    // cause rename to fail with permission denied at high rotation rates.
    static bool try_rename_(const std::filesystem::path &src, const std::filesystem::path &dst) {
        constexpr int max_attempts = 5;
        constexpr auto retry_delay = std::chrono::milliseconds(20);
        std::error_code ec;
        for (int attempt = 0; attempt < max_attempts; ++attempt) {
            std::filesystem::remove(dst, ec);  // best-effort
            std::filesystem::rename(src, dst, ec);
            if (!ec) return true;
            std::this_thread::sleep_for(retry_delay);
        }
        return false;
    }

    // walk the parent directory; return the highest archive index found, and
    // delete any archives below the [highest - max_files + 1, highest] window
    // so the file-count invariant holds even if the chain had holes from an
    // earlier run (e.g. user-deleted or different max_files settings).
    // returns 0 if no archives exist (next_index_ becomes 1 on first rotation).
    // called only once in construction.
    std::size_t scan_and_prune_archives_() const {
        auto parent = base_filename_.parent_path();
        if (parent.empty()) parent = ".";
        std::error_code ec;
        if (!std::filesystem::exists(parent, ec)) return 0;
        const auto stem = base_filename_.stem().string();
        const auto ext = base_filename_.extension().string();

        // first pass: find the highest index
        std::size_t highest = 0;
        for (const auto &entry : std::filesystem::directory_iterator(parent, ec)) {
            if (ec) break;
            if (auto n = parse_archive_index_(entry.path().filename().string(), stem, ext); n.has_value()) {
                if (*n > highest) highest = *n;
            }
        }
        if (highest == 0) return 0;  // no archives found, nothing to prune

        // second pass: delete anything below the keep window (best-effort)
        if (highest > max_files_) {
            const std::size_t lowest_keep = highest - max_files_ + 1;
            for (const auto &entry : std::filesystem::directory_iterator(parent, ec)) {
                if (ec) break;
                if (auto n = parse_archive_index_(entry.path().filename().string(), stem, ext); n.has_value()) {
                    if (*n < lowest_keep) std::filesystem::remove(entry.path(), ec);
                }
            }
        }
        return highest;
    }

    // does `filename` match `<base_stem>.<digits><base_ext>`? returns the index if so.
    static std::optional<std::size_t> parse_archive_index_(const std::string &filename,
                                                           const std::string &base_stem,
                                                           const std::string &base_ext) {
        // strip ext
        if (filename.size() <= base_ext.size()) return std::nullopt;
        if (!base_ext.empty() && filename.compare(filename.size() - base_ext.size(), base_ext.size(), base_ext) != 0)
            return std::nullopt;
        const auto without_ext = filename.substr(0, filename.size() - base_ext.size());
        // expect: base_stem + "." + digits
        const auto prefix = base_stem + ".";
        if (without_ext.size() <= prefix.size()) return std::nullopt;
        if (without_ext.compare(0, prefix.size(), prefix) != 0) return std::nullopt;
        const auto digits = without_ext.substr(prefix.size());
        for (char c : digits) {
            if (c < '0' || c > '9') return std::nullopt;
        }
        try {
            return std::stoull(digits);
        } catch (...) {
            return std::nullopt;
        }
    }
};

}  // namespace spdlite
