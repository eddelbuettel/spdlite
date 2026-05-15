// SPDX-License-Identifier: MIT

#include <doctest/doctest.h>

#include <filesystem>
#include <fstream>
#include <sstream>
#include <string>

#include "spdlite/logger.h"
#include "spdlite/sinks/file_sink.h"
#include "support/tmpdir.h"

using namespace spdlite;
namespace fs = std::filesystem;

static std::string read_all(const fs::path& p) {
    std::ifstream in(p, std::ios::binary);
    std::ostringstream ss;
    ss << in.rdbuf();
    return ss.str();
}

TEST_CASE("file_sink creates the file on construction") {
    spdlite_test::tmpdir td{"file_create"};
    const auto path = td / "out.txt";
    {
        file_sink sink{path};
    }
    CHECK(fs::exists(path));
}

TEST_CASE("file_sink auto-creates parent directories") {
    spdlite_test::tmpdir td{"file_parents"};
    const auto path = td.path() / "a" / "b" / "c" / "out.txt";
    {
        file_sink sink{path};
    }
    CHECK(fs::exists(path));
    CHECK(fs::is_directory(td.path() / "a" / "b" / "c"));
}

TEST_CASE("file_sink truncate mode discards existing contents") {
    spdlite_test::tmpdir td{"file_trunc"};
    const auto path = td / "out.txt";
    {
        std::ofstream pre(path, std::ios::binary);
        pre << "preexisting\n";
    }
    REQUIRE(read_all(path) == "preexisting\n");

    {
        logger_st<file_sink> log{"x", file_sink{path, open_mode::truncate}};
        log.info("fresh");
    }

    auto contents = read_all(path);
    CHECK(contents.find("preexisting") == std::string::npos);
    CHECK(contents.find("fresh") != std::string::npos);
}

TEST_CASE("file_sink append mode preserves existing contents") {
    spdlite_test::tmpdir td{"file_append"};
    const auto path = td / "out.txt";
    {
        std::ofstream pre(path, std::ios::binary);
        pre << "preexisting\n";
    }

    {
        logger_st<file_sink> log{"x", file_sink{path}};  // default = append
        log.info("added");
    }

    auto contents = read_all(path);
    CHECK(contents.find("preexisting") != std::string::npos);
    CHECK(contents.find("added") != std::string::npos);
}

TEST_CASE("file_sink writes the formatted line (header + payload + newline)") {
    spdlite_test::tmpdir td{"file_write"};
    const auto path = td / "out.txt";
    {
        logger_st<file_sink> log{"name", file_sink{path, open_mode::truncate}};
        log.info("hello");
        log.warn("there");
    }
    auto contents = read_all(path);
    CHECK(contents.find("[name]") != std::string::npos);
    CHECK(contents.find("[INF] hello") != std::string::npos);
    CHECK(contents.find("[WRN] there") != std::string::npos);
    CHECK(contents.back() == '\n');
}

TEST_CASE("file_sink throws when the path cannot be opened") {
    // Force a failure by making the parent path traverse through an existing regular file:
    // create_directories then fails with filesystem_error (a std::runtime_error subclass).
    spdlite_test::tmpdir td{"file_bad"};
    const auto blocker = td / "blocker";
    {
        std::ofstream o(blocker, std::ios::binary);
        o << "x";
    }
    const auto bad_path = blocker / "child" / "out.txt";
    CHECK_THROWS_AS(file_sink{bad_path}, std::runtime_error);
}
