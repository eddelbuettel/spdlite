cmake_minimum_required(VERSION 3.23)
project(spdlite LANGUAGES CXX)

if(NOT CMAKE_BUILD_TYPE AND NOT CMAKE_CONFIGURATION_TYPES)
    set(CMAKE_BUILD_TYPE Release CACHE STRING "Build type" FORCE)
endif()

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

# Header files (for IDE visibility only - not compiled)
set(SPDLITE_HEADERS
    include/spdlite/common.h
    include/spdlite/formatter.h
    include/spdlite/logger.h
    include/spdlite/sinks/file_sink.h
    include/spdlite/sinks/null_sink.h
    include/spdlite/sinks/stdout_sink.h
    include/spdlite/sinks/color_sink.h)

# spdlite is header-only (fmt bundled)
add_library(spdlite INTERFACE)
target_include_directories(spdlite INTERFACE
    $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
    $<INSTALL_INTERFACE:include>)
target_compile_features(spdlite INTERFACE cxx_std_20)
if(MSVC)
    target_compile_options(spdlite INTERFACE /utf-8)
endif()

# Example
option(SPDLITE_BUILD_EXAMPLE "Build spdlite example" ON)
if(SPDLITE_BUILD_EXAMPLE)
    add_executable(example example/example.cpp ${SPDLITE_HEADERS})
    source_group("Header Files\\spdlite" FILES ${SPDLITE_HEADERS})
    target_link_libraries(example PRIVATE spdlite)
endif()

# Caller-zoo: synthetic stress for measuring per-callsite cost (binary size, compile time).
option(SPDLITE_BUILD_CALLER_ZOO "Build caller_zoo stress executable" OFF)
if(SPDLITE_BUILD_CALLER_ZOO)
    add_executable(caller_zoo bench/caller_zoo.cpp)
    target_link_libraries(caller_zoo PRIVATE spdlite)
endif()

# Benchmarks
option(SPDLITE_BUILD_BENCH "Build spdlite benchmarks" OFF)
if(SPDLITE_BUILD_BENCH)
    find_package(benchmark CONFIG)
    if(NOT benchmark_FOUND)
        message(STATUS "Downloading Google Benchmark")
        include(FetchContent)
        set(BENCHMARK_ENABLE_TESTING OFF CACHE INTERNAL "")
        FetchContent_Declare(googlebenchmark
            GIT_REPOSITORY https://github.com/google/benchmark.git
            GIT_TAG v1.9.5)
        FetchContent_MakeAvailable(googlebenchmark)
    endif()

    add_executable(latency bench/latency.cpp)
    target_link_libraries(latency PRIVATE spdlite benchmark::benchmark)

    add_executable(formatter_bench bench/formatter_bench.cpp)
    target_link_libraries(formatter_bench PRIVATE spdlite benchmark::benchmark)

    # Comparative benchmark vs spdlog (optional)
    if(EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/../spdlog/CMakeLists.txt")
        add_subdirectory(${CMAKE_CURRENT_SOURCE_DIR}/../spdlog ${CMAKE_BINARY_DIR}/spdlog_build EXCLUDE_FROM_ALL)
        add_executable(vs_spdlog bench/vs_spdlog.cpp)
        # both header-only - spdlite's fmt (FMT_HEADER_ONLY) and spdlog's inline code share the same symbols cleanly
        target_link_libraries(vs_spdlog PRIVATE spdlite spdlog::spdlog_header_only benchmark::benchmark)
    endif()
endif()
