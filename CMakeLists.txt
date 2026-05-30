cmake_minimum_required(VERSION 3.15)
project(ChessEngine)

# C++ Standard
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Compiler flags
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wall -Wextra -mbmi2")
set(CMAKE_CXX_FLAGS_RELEASE "-fopenmp -O3 -march=native -mbmi2 -mcmodel=medium -DNDEBUG")
set(CMAKE_CXX_FLAGS_DEBUG "-g -fopenmp -O0 -mcmodel=medium")

# Include directories
include_directories(
    ${CMAKE_SOURCE_DIR}/src/include
)

# Generate Compile Commands

set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

# Source files
set(ENGINE_SOURCES
    src/board_manager/board_manager.cpp
    src/lookup_tables/lookup_tables.cpp
    src/search/search.cpp
    src/evaluate/evaluate.cpp
)

# Create engine library (reusable for main and tests)
add_library(chess_engine_lib ${ENGINE_SOURCES})

# Main executable
add_executable(chess_engine src/Main/main.cpp)
target_link_libraries(chess_engine chess_engine_lib)

# Enable testing
enable_testing()

# Test executables
add_executable(test_movegen tests/test_move_making.cpp)
target_link_libraries(test_movegen chess_engine_lib)
add_test(NAME MoveGenTest COMMAND test_movegen)
