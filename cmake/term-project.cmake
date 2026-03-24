project(term-project C)

# Add executable with source files from projects/term_project/src/
file(GLOB_RECURSE TERM_PROJECT_SOURCES "${CMAKE_SOURCE_DIR}/projects/term_project/src/*.c")

link_libraries("-static")

add_executable(term-project ${TERM_PROJECT_SOURCES})

# Add include directories if needed
target_include_directories(term-project PRIVATE
    "${CMAKE_SOURCE_DIR}/projects/term_project/src"
    "${CMAKE_SOURCE_DIR}/projects/term_project/src/utils"
    "${CMAKE_SOURCE_DIR}/projects/term_project/src/vent_control"
    "${CMAKE_SOURCE_DIR}/projects/term_project/src/sensor_monitoring"
    "${CMAKE_SOURCE_DIR}/projects/term_project/src/log_handler"
    "${CMAKE_SOURCE_DIR}/projects/term_project/src/supervisor_input"
)
