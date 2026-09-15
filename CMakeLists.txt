cmake_minimum_required(VERSION 3.20)
project(GraphFlow VERSION 1.0.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

# Optimization and compiler flags
if (CMAKE_CXX_COMPILER_ID MATCHES "GNU|Clang")
    add_compile_options(-Wall -Wextra -Wpedantic -O3 -march=native)
elseif (MSVC)
    add_compile_options(/W4 /O2 /permissive-)
endif()

find_package(Threads REQUIRED)

include_directories(
    ${CMAKE_CURRENT_SOURCE_DIR}/include
)

# Core Library
add_library(graphflow_core STATIC
    src/core/arena_allocator.cpp
    src/graph/graph.cpp
    src/graph/bitset_adj_list.cpp
    src/graph/graph_generator.cpp
    src/algorithms/shortest_path.cpp
    src/algorithms/max_flow.cpp
    src/algorithms/dynamic_programming.cpp
)

target_link_libraries(graphflow_core PUBLIC Threads::Threads)

# Main Executable
add_executable(graphflow_cli src/main.cpp)
target_link_libraries(graphflow_cli PRIVATE graphflow_core)

# Tests
enable_testing()

macro(add_graphflow_test TEST_NAME SRC_FILE)
    add_executable(${TEST_NAME} ${SRC_FILE})
    target_link_libraries(${TEST_NAME} PRIVATE graphflow_core)
    add_test(NAME ${TEST_NAME} COMMAND ${TEST_NAME})
endmacro()

add_graphflow_test(test_arena tests/test_arena.cpp)
add_graphflow_test(test_heap tests/test_heap.cpp)
add_graphflow_test(test_graph tests/test_graph.cpp)
add_graphflow_test(test_shortest_path tests/test_shortest_path.cpp)
add_graphflow_test(test_max_flow tests/test_max_flow.cpp)
add_graphflow_test(test_dynamic_programming tests/test_dynamic_programming.cpp)
add_graphflow_test(test_concurrent_engine tests/test_concurrent_engine.cpp)

# Benchmarks
macro(add_graphflow_bench BENCH_NAME SRC_FILE)
    add_executable(${BENCH_NAME} ${SRC_FILE})
    target_link_libraries(${BENCH_NAME} PRIVATE graphflow_core)
endmacro()

add_graphflow_bench(bench_pathfinding benchmarks/bench_pathfinding.cpp)
add_graphflow_bench(bench_allocator benchmarks/bench_allocator.cpp)
add_graphflow_bench(bench_flow benchmarks/bench_flow.cpp)
