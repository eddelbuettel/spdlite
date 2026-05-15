// SPDX-License-Identifier: MIT

// Per-test temp directory under <system_tmp>/spdlite_tests/<unique>/.
// Sample folder: /tmp/spdlite_tests/rot_basic_18472938472013_0
// Cleaned up on destruction (best-effort).

#pragma once

#include <atomic>
#include <chrono>
#include <filesystem>
#include <string>
#include <system_error>

namespace spdlite_test {

class tmpdir {
public:
    explicit tmpdir(std::string_view label) {
        static std::atomic<unsigned> counter{0};
        const auto stamp = std::chrono::steady_clock::now().time_since_epoch().count();
        path_ = std::filesystem::temp_directory_path() / "spdlite_tests" /
                (std::string{label} + "_" + std::to_string(stamp) + "_" + std::to_string(counter.fetch_add(1)));
        std::error_code ec;
        std::filesystem::remove_all(path_, ec);
        std::filesystem::create_directories(path_, ec);
    }

    ~tmpdir() {
        std::error_code ec;
        std::filesystem::remove_all(path_, ec);
    }

    tmpdir(const tmpdir&) = delete;
    tmpdir& operator=(const tmpdir&) = delete;

    [[nodiscard]] const std::filesystem::path& path() const noexcept { return path_; }
    [[nodiscard]] std::filesystem::path operator/(std::string_view rel) const { return path_ / rel; }

private:
    std::filesystem::path path_;
};

}  // namespace spdlite_test
