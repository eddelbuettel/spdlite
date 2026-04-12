// spdlite example

#include "spdlite/logger.h"
#include "spdlite/sinks/basic_file_sink.h"
#include "spdlite/sinks/stdout_color_sink.h"
#include "spdlite/sinks/stdout_sink.h"

int main() {
    using namespace spdlite;
    // Color console logger (single-threaded)
    logger_st<sinks::stdout_color_sink> console("app");
    console.info("Hello {}", "world");
    console.info("Value: {}", 42);
    console.debug("This should not appear (level is info)");

    console.set_level(level::trace);
    console.trace("Trace message");
    console.debug("Debug message");
    console.info("Info message");
    console.warn("Warning message");
    console.error("Error message");
    console.critical("Critical message");

    // File logger
    logger_st<sinks::basic_file_sink> file_logger("file", sinks::basic_file_sink{"logs/example.txt", true});
    file_logger.info("Written to file");
    file_logger.error("Error written to file: {}", 404);

    // Multiple sinks — color console + file
    logger_st<sinks::stdout_color_sink, sinks::basic_file_sink> multi("multi", sinks::stdout_color_sink{},
                                                                      sinks::basic_file_sink{"logs/multi.txt", true});
    multi.info("Goes to both console and file");
    multi.warn("Warning: {}", "something happened");

    // String view overload (no formatting)
    console.info("Plain string, no formatting");

    // Logger without a name
    logger_st<sinks::stdout_color_sink> anon;
    anon.info("No logger name in the output");

    return 0;
}
