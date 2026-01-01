cmake_minimum_required(VERSION 3.15)
project(ChessEngine)

# C++ Standard
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Compiler flags
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wall -Wextra -mbmi2")
set(CMAKE_CXX_FLAGS_RELEASE "-O3 -march=native -mbmi2 -DNDEBUG")
set(CMAKE_CXX_FLAGS_DEBUG "-g -O0")

# Include directories
include_directories(
    ${CMAKE_SOURCE_DIR}/board_manager
    ${CMAKE_SOURCE_DIR}/lookup_tables
    ${CMAKE_SOURCE_DIR}/move_encoding
    ${CMAKE_SOURCE_DIR}/search
)

# Source files
set(ENGINE_SOURCES
    board_manager/board_manager.cpp
    lookup_tables/lookup_tables.cpp
)

# Create engine library (reusable for main and tests)
add_library(chess_engine_lib ${ENGINE_SOURCES})

# Main executable
add_executable(chess_engine main.cpp)
target_link_libraries(chess_engine chess_engine_lib)

# Enable testing
enable_testing()

# Test executables
add_executable(test_movegen tests/one_test.cpp)
target_link_libraries(test_movegen chess_engine_lib)
add_test(NAME MoveGenTest COMMAND test_movegen)
