project(p1 C)

# Add executable with source files from projects/project1/src/
file(GLOB_RECURSE P1_SOURCES "${CMAKE_SOURCE_DIR}/projects/project1/src/*.c")

link_libraries("-static")

add_executable(p1 ${P1_SOURCES})

# Add include directories if needed
target_include_directories(p1 PRIVATE
    "${CMAKE_SOURCE_DIR}/projects/project1/src"
    "${CMAKE_SOURCE_DIR}/projects/project1/src/utils"
)
