cmake_minimum_required(VERSION 3.20)
project(orderbook VERSION 1.0.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

if(CMAKE_CXX_COMPILER_ID MATCHES "Clang" OR CMAKE_CXX_COMPILER_ID MATCHES "AppleClang")
	add_compile_options(
		-Wall -Wextra -Wpedantic
		-Wno-unused-parameter
		$<$<CONFIG:Release>:-O3>
        $<$<CONFIG:Release>:-march=native>
        $<$<CONFIG:Debug>:-O0>
        $<$<CONFIG:Debug>:-g>
        $<$<CONFIG:Debug>:-fsanitize=thread>
	)
	add_link_options(
		$<$<CONFIG:Debug>:-fsanitize=thread>
	)
endif()

find_program(CCACHE_PROGRAM ccache)
if(CCACHE_PROGRAM)
	set(CMAKE_CXX_COMPILER_LAUNCHER ${CCACHE_PROGRAM})
endif()

include(FetchContent)

FetchContent_Declare(
	googletest
	GIT_REPOSITORY https://github.com/google/googletest.git
	GIT_TAG v1.14.0
)

FetchContent_MakeAvailable(googletest)

FetchContent_Declare(
	benchmark
	GIT_REPOSITORY https://github.com/google/benchmark.git
    GIT_TAG v1.8.3
)

set(BENCHMARK_ENABLE_TESTING OFF CACHE BOOL "" FORCE)
FetchContent_MakeAvailable(benchmark)


add_library(orderbook_lib
	src/order_book.cpp
	src/matching_engine.cpp
	src/timer.cpp
)

target_include_directories(orderbook_lib PUBLIC include)

add_executable(orderbook_main src/main.cpp)
target_link_libraries(orderbook_main PRIVATE orderbook_lib)

enable_testing()
add_executable(orderbook_tests
	tests/test_order_book.cpp
	tests/test_matching.cpp
)

target_link_libraries(orderbook_tests PRIVATE orderbook_lib GTest::gtest_main)
include(GoogleTest)
gtest_discover_tests(orderbook_tests)

add_executable(bench_insert bench/bench_insert.cpp)
target_link_libraries(bench_insert PRIVATE orderbook_lib benchmark::benchmark)

add_executable(bench_cancel bench/bench_cancel.cpp)
target_link_libraries(bench_cancel PRIVATE orderbook_lib benchmark::benchmark)

add_executable(bench_match bench/bench_match.cpp)
target_link_libraries(bench_match PRIVATE orderbook_lib benchmark::benchmark)







