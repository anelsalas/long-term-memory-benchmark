cmake_minimum_required(VERSION 3.20)
project(LongTermMemoryBenchmark VERSION 0.1.0 LANGUAGES CXX)

set(ltm_top_level OFF)
if(CMAKE_SOURCE_DIR STREQUAL PROJECT_SOURCE_DIR)
  set(ltm_top_level ON)
endif()
option(LONG_TERM_MEMORY_BENCHMARK_BUILD_TESTS "Build memory component tests" ${ltm_top_level})

add_library(long_term_memory_benchmark STATIC src/memory.cpp src/report.cpp)
add_library(LongTermMemoryBenchmark::core ALIAS long_term_memory_benchmark)
target_compile_features(long_term_memory_benchmark PUBLIC cxx_std_17)
target_include_directories(long_term_memory_benchmark PUBLIC
  $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>)
if(MSVC)
  target_compile_options(long_term_memory_benchmark PRIVATE /W4 /permissive-)
else()
  target_compile_options(long_term_memory_benchmark PRIVATE -Wall -Wextra -Wpedantic)
endif()

add_executable(long-term-memory-benchmark src/main.cpp)
target_link_libraries(long-term-memory-benchmark PRIVATE LongTermMemoryBenchmark::core)

if(LONG_TERM_MEMORY_BENCHMARK_BUILD_TESTS)
  enable_testing()
  add_executable(ltm_component_tests tests/component_tests.cpp)
  target_link_libraries(ltm_component_tests PRIVATE LongTermMemoryBenchmark::core)
  foreach(case IN ITEMS exact_source correction_order person_filter unknown_query
      top_k as_of duplicate_id empty_record punctuation score_tie invalid_timestamp
      report_escape report_fraction report_invalid_time)
    add_test(NAME ltm.unit.${case} COMMAND ltm_component_tests ${case})
    set_tests_properties(ltm.unit.${case} PROPERTIES LABELS "memory;component;unit;offline")
  endforeach()
  add_test(NAME ltm.integration.offline_demo COMMAND long-term-memory-benchmark --offline-demo)
  set_tests_properties(ltm.integration.offline_demo PROPERTIES
    LABELS "memory;integration;offline" PASS_REGULAR_EXPRESSION "offline_component")
endif()
