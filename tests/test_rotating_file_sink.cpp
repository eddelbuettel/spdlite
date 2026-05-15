// SPDX-License-Identifier: MIT

#include <doctest/doctest.h>

#include <algorithm>
#include <filesystem>
#include <fstream>
#include <stdexcept>
#include <string>

#include "spdlite/logger.h"
#include "spdlite/sinks/rotating_file_sink.h"
#include "support/tmpdir.h"

using namespace spdlite;
namespace fs = std::filesystem;

static void touch(const fs::path& p, std::string_view content = "x") {
    fs::create_directories(p.parent_path());
    std::ofstream out(p, std::ios::binary);
    out << content;
}

TEST_CASE("max_size=0 throws") {
    spdlite_test::tmpdir td{"rot_zero"};
    CHECK_THROWS_AS(rotating_file_sink(td / "x.txt", 0, 1), std::invalid_argument);
}

TEST_CASE("max_files greater than max_files_limit throws") {
    spdlite_test::tmpdir td{"rot_limit"};
    CHECK_THROWS_AS(rotating_file_sink(td / "x.txt", 1024, rotating_file_sink::max_files_limit + 1), std::invalid_argument);
}

TEST_CASE("max_files_limit constant matches documented value") { CHECK(rotating_file_sink::max_files_limit == 1000); }

TEST_CASE("rotating_file_sink creates the active file") {
    spdlite_test::tmpdir td{"rot_create"};
    const auto path = td / "app.txt";
    {
        rotating_file_sink sink{path, 1024, 3};
    }
    CHECK(fs::exists(path));
}

TEST_CASE("rotation produces an archive once max_size is exceeded") {
    spdlite_test::tmpdir td{"rot_basic"};
    const auto path = td / "app.txt";
    {
        // 4 ~55-byte log lines exceed the 128-byte cap on the 3rd write, producing exactly one
        // rotation. max_files=10 ensures the resulting app.1.txt isn't pruned out of the window.
        logger_st<rotating_file_sink> log{"r", rotating_file_sink{path, 128, 10}};
        for (int i = 0; i < 4; ++i) log.info("padding line number {}", i);
    }

    CHECK(fs::exists(path));
    CHECK(fs::exists(td / "app.1.txt"));
}

TEST_CASE("max_files window keeps only the newest N archives") {
    spdlite_test::tmpdir td{"rot_window"};
    const auto path = td / "app.txt";
    {
        // many rotations under a window of 2; only the two newest archives should survive
        logger_st<rotating_file_sink> log{"r", rotating_file_sink{path, 128, 2}};
        for (int i = 0; i < 60; ++i) log.info("padding line number {}", i);
    }

    CHECK(fs::exists(path));

    // For "app.N.txt": path.stem() = "app.N", path.stem().extension() = ".N".
    std::size_t archive_count = 0;
    std::size_t highest = 0;
    for (const auto& entry : fs::directory_iterator(td.path())) {
        if (entry.path().filename() == "app.txt") continue;
        ++archive_count;
        const auto dot_n = entry.path().stem().extension().string();  // ".N"
        if (dot_n.size() > 1) highest = std::max<std::size_t>(highest, std::stoul(dot_n.substr(1)));
    }
    CHECK(archive_count == 2);
    CHECK(fs::exists(td / ("app." + std::to_string(highest) + ".txt")));
    CHECK(fs::exists(td / ("app." + std::to_string(highest - 1) + ".txt")));
}

TEST_CASE("scan_and_prune drops archives below the keep window on construction") {
    spdlite_test::tmpdir td{"rot_prune"};
    const auto path = td / "app.txt";

    // pre-populate stale archives
    touch(td / "app.1.txt");
    touch(td / "app.2.txt");
    touch(td / "app.3.txt");
    touch(td / "app.4.txt");
    touch(td / "app.5.txt");

    {
        // max_files=2 so keep window is [highest-1, highest] = [4, 5]
        rotating_file_sink sink{path, 1024, 2};
    }

    CHECK_FALSE(fs::exists(td / "app.1.txt"));
    CHECK_FALSE(fs::exists(td / "app.2.txt"));
    CHECK_FALSE(fs::exists(td / "app.3.txt"));
    CHECK(fs::exists(td / "app.4.txt"));
    CHECK(fs::exists(td / "app.5.txt"));
}

TEST_CASE("counter resumes across restarts") {
    spdlite_test::tmpdir td{"rot_resume"};
    const auto path = td / "app.txt";

    // pre-populate as if a previous run rotated to .5
    touch(td / "app.5.txt");

    {
        // construct, write enough to trigger one rotation
        logger_st<rotating_file_sink> log{"r", rotating_file_sink{path, 128, 10}};
        for (int i = 0; i < 10; ++i) log.info("padding line number {}", i);
    }

    // next archive after .5 should be .6 (counter resumed)
    CHECK(fs::exists(td / "app.6.txt"));
}

TEST_CASE("base filename with no parent directory still works") {
    // The scan_and_prune code special-cases an empty parent path; exercise that branch
    // by passing a bare filename (cwd-relative). Cleanup is manual since we're not in a tmpdir.
    const auto unique =
        std::string{"spdlite_rot_noparent_"} + std::to_string(std::chrono::steady_clock::now().time_since_epoch().count());
    const fs::path base = unique + ".log";
    {
        rotating_file_sink sink{base, 1024, 1};
    }
    CHECK(fs::exists(base));
    std::error_code ec;
    fs::remove(base, ec);
}
