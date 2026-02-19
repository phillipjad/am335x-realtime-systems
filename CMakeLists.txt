cmake_minimum_required(VERSION 3.10)
project(swe660)

# Per https://sourceware.org/glibc/manual/2.43/html_mono/libc.html#index-_005fPOSIX_005fC_005fSOURCE
add_definitions(-D_POSIX_C_SOURCE=202405L)

# Define C standard being used -- Using C23 for now but should probably check with Prof
set(CMAKE_C_STANDARD 23)
set(CMAKE_C_STANDARD_REQUIRED ON)
set(CMAKE_C_EXTENSIONS OFF)

# Set build type - release or debug
if(CMAKE_BUILD_TYPE STREQUAL "Debug")
	message(STATUS "Global Build Mode: DEBUG")
	add_definitions(-DDEBUG)
    add_compile_options(-g -O0)
else()
	message(STATUS "Global Build Mode: RELEASE")
    add_compile_options(-O2)
endif()

# Set default target if not specified
if(NOT DEFINED TARGET)
    message(STATUS "No TARGET specified. Use -DTARGET=P[n] to build a specific project.")
    return()
endif()

# Set configuration input type
if(USE_CONFIG)
    message(STATUS "Setting parameters from configuration file")
    add_definitions(-DUSE_CONFIG)
endif()

# Master list of all options to pass to the compiler
set(COMPILER_OPTIONS
    -Wall
    -Wextra
    -Wpedantic
    -Werror
    -Wformat=2
    -Wformat-security
    -Wshadow
    -Wundef
    -Wmissing-prototypes
    -Wmissing-declarations
    -Wstrict-prototypes
    -Wcast-qual
    -Wcast-align
    -Wvla
)

# Pretty print all compiler options that are being set
string(REPLACE ";" "\n\t" COMPILER_OPTIONS_STR "${COMPILER_OPTIONS}")
set(COMPILER_OPTIONS_STR "\t${COMPILER_OPTIONS_STR}")
message(STATUS "Passing compile options:\n${COMPILER_OPTIONS_STR}")

# Add each option in COMPILER_OPTIONS
foreach(opt IN LISTS COMPILER_OPTIONS)
    add_compile_options(${opt})
endforeach()


# Include the appropriate project cmake file
if("${TARGET}" MATCHES "^P[0-9]+$")
    string(TOLOWER "${TARGET}" target_lower)
    set(PROJECT_CMAKE_FILE "${CMAKE_SOURCE_DIR}/cmake/${target_lower}.cmake")

    if(EXISTS "${PROJECT_CMAKE_FILE}")
        include("${PROJECT_CMAKE_FILE}")
    else()
        message(FATAL_ERROR "Project file not found: ${PROJECT_CMAKE_FILE}")
    endif()
else()
    message(FATAL_ERROR "Invalid TARGET: ${TARGET}. Use format P1, P2, etc.")
endif()
