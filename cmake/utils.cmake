# Read spdlite version from include/spdlite/common.h and expose it via
# SPDLITE_VERSION{,_MAJOR,_MINOR,_PATCH} in the caller's scope.
function(spdlite_extract_version)
    file(READ "${CMAKE_CURRENT_LIST_DIR}/include/spdlite/common.h" file_contents)

    string(REGEX MATCH "SPDLITE_VER_MAJOR ([0-9]+)" _ "${file_contents}")
    if(NOT CMAKE_MATCH_COUNT EQUAL 1)
        message(FATAL_ERROR "Could not extract major version number from spdlite/common.h")
    endif()
    set(ver_major ${CMAKE_MATCH_1})

    string(REGEX MATCH "SPDLITE_VER_MINOR ([0-9]+)" _ "${file_contents}")
    if(NOT CMAKE_MATCH_COUNT EQUAL 1)
        message(FATAL_ERROR "Could not extract minor version number from spdlite/common.h")
    endif()
    set(ver_minor ${CMAKE_MATCH_1})

    string(REGEX MATCH "SPDLITE_VER_PATCH ([0-9]+)" _ "${file_contents}")
    if(NOT CMAKE_MATCH_COUNT EQUAL 1)
        message(FATAL_ERROR "Could not extract patch version number from spdlite/common.h")
    endif()
    set(ver_patch ${CMAKE_MATCH_1})

    set(SPDLITE_VERSION_MAJOR ${ver_major} PARENT_SCOPE)
    set(SPDLITE_VERSION_MINOR ${ver_minor} PARENT_SCOPE)
    set(SPDLITE_VERSION_PATCH ${ver_patch} PARENT_SCOPE)
    set(SPDLITE_VERSION "${ver_major}.${ver_minor}.${ver_patch}" PARENT_SCOPE)
endfunction()
