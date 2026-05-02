// SPDX-License-Identifier: MIT
// Copyright (c) 2026, Gabi Melman

#include "spdlite/sinks/basic_file_sink.h"
#include "spdlite/sinks/color_sink.h"
#include "spdlite/sinks/stdout_sink.h"
#include "spdlite/spdlite.h"

void log_levels();

int main() {
    using namespace spdlite;
    using sinks::color_stdout;
    using sinks::basic_file_sink;   

    // Color console logger (single-threaded)
    logger_st<color_stdout> console;
    console.info(R"(
                ____ ___ __
   _________  ____/ / (.) /____
  / ___/ __ \/ __  / / / __/ _ \
 (__  ) /_/ / /_/ / / / /_/  __/
/____/ .___/\__,_/_/_/\__/\___/   v{}.{}.{}
    /_/
)",
                 SPDLITE_VER_MAJOR, SPDLITE_VER_MINOR, SPDLITE_VER_PATCH);

    // File logger
    logger_st<basic_file_sink> file_logger("my_logger", basic_file_sink{"logs/example.txt", true});
    file_logger.info("This is a log message in the file");

    // Multiple sinks — color console + file
    logger_st<color_stdout, basic_file_sink> multi("my_logger", color_stdout{},
                                                   basic_file_sink{"logs/multi.txt", true});
    multi.info("This goes to both console and file");

    log_levels();
    return 0;
}

// Log messages at different levels.
// By default, levels>=info are enabled, but this can be configured at compile-time or runtime.
void log_levels() {    
    using spdlite::sinks::color_stdout;
    using spdlite::logger_st<color_stdout> console;
    console.log_level(level::trace);  // enable trace and above
    console.trace("This is a {} message", "trace");
    console.debug("This is a {} message", "debug");
    console.info("This is a {} message", "info");
    console.warn("This is a {} message", "warning");
    console.error("This is a {} message", "error");
    console.critical("This is a {} message", "critical");
}
