// SPDX-License-Identifier: MIT
// Copyright (c) 2026, Gabi Melman

#include "spdlite/sinks/color_sink.h"
#include "spdlite/sinks/file_sink.h"
#include "spdlite/sinks/stdout_sink.h"
#include "spdlite/spdlite.h"

void banner();
void log_levels();
void file_sink_example();
void multi_sink_example();

int main() {
    banner();
    log_levels();
    file_sink_example();
    multi_sink_example();
    return 0;
}

// Print an ASCII banner with the spdlite version through a colored console logger.
void banner() {
    using namespace spdlite;
    logger_st<console_sink> console;
    console.info(R"(
                ____ ___ __
   _________  ____/ / (.) /____
  / ___/ __ \/ __  / / / __/ _ \
 (__  ) /_/ / /_/ / / / /_/  __/
/____/ .___/\__,_/_/_/\__/\___/   v{}.{}.{}
    /_/
)",
                 SPDLITE_VER_MAJOR, SPDLITE_VER_MINOR, SPDLITE_VER_PATCH);
}

// Walk all log levels through a colored console logger.
// By default the threshold is info; enable trace explicitly to see everything.
void log_levels() {
    using namespace spdlite;
    logger_st console("", console_sink{});
    console.log_level(level::trace);
    console.trace("This is a {} message", "trace");
    console.debug("This is a {} message", "debug");
    console.info("This is a {} message", "info");
    console.warn("This is a {} message", "warning");
    console.error("This is a {} message", "error");
    console.critical("This is a {} message", "critical");
}

// Log to a file via file_sink. The sink creates parent directories
// automatically and uses _wfopen on Windows for Unicode paths.
void file_sink_example() {
    using namespace spdlite;
    logger_st<file_sink> file_logger("my_logger", file_sink{"logs/example.txt", true});
    file_logger.info("This message is written to logs/example.txt");
}

// Compose multiple sinks into one logger - a single log call writes to all of them.
void multi_sink_example() {
    using namespace spdlite;
    logger_st<console_sink, file_sink> multi("my_logger", console_sink{}, file_sink{"logs/multi.txt", true});
    multi.info("This goes to both console and file");
}
