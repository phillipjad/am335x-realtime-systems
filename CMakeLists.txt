cmake_minimum_required(VERSION 3.10)
project(swe660)

# Set default target if not specified
if(NOT DEFINED TARGET)
    message(STATUS "No TARGET specified. Use -DTARGET=P[n] to build a specific project.")
    return()
endif()

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
