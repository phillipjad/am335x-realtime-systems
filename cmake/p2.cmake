project(p2 C)

# Add executable with source files from projects/project2/src/
file(GLOB_RECURSE P2_SOURCES "${CMAKE_SOURCE_DIR}/projects/project2/src/*.c")

link_libraries("-static")

add_executable(p2 ${P2_SOURCES})

# Add include directories if needed
target_include_directories(p2 PRIVATE
    "${CMAKE_SOURCE_DIR}/projects/project2/src"
    "${CMAKE_SOURCE_DIR}/projects/project2/src/utils"
)
