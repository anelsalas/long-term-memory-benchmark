cmake_minimum_required(VERSION 3.20)
project(DumasMemoryBenchmark VERSION 0.1.0 LANGUAGES CXX)

set(dmb_top_level OFF)
if(CMAKE_SOURCE_DIR STREQUAL PROJECT_SOURCE_DIR)
  set(dmb_top_level ON)
endif()
option(DUMAS_MEMORY_BENCHMARK_BUILD_TESTS "Build memory component tests" ${dmb_top_level})

add_library(dumas_memory_benchmark STATIC src/memory.cpp src/report.cpp)
add_library(DumasMemoryBenchmark::core ALIAS dumas_memory_benchmark)
target_compile_features(dumas_memory_benchmark PUBLIC cxx_std_17)
target_include_directories(dumas_memory_benchmark PUBLIC
  $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>)
if(MSVC)
  target_compile_options(dumas_memory_benchmark PRIVATE /W4 /permissive-)
else()
  target_compile_options(dumas_memory_benchmark PRIVATE -Wall -Wextra -Wpedantic)
endif()

add_executable(dumas-memory-benchmark src/main.cpp)
target_link_libraries(dumas-memory-benchmark PRIVATE DumasMemoryBenchmark::core)

if(DUMAS_MEMORY_BENCHMARK_BUILD_TESTS)
  enable_testing()
  add_executable(dmb_component_tests tests/component_tests.cpp)
  target_link_libraries(dmb_component_tests PRIVATE DumasMemoryBenchmark::core)
  foreach(case IN ITEMS exact_source correction_order person_filter unknown_query
      top_k as_of duplicate_id empty_record punctuation score_tie invalid_timestamp
      report_escape report_fraction report_invalid_time)
    add_test(NAME dmb.unit.${case} COMMAND dmb_component_tests ${case})
    set_tests_properties(dmb.unit.${case} PROPERTIES LABELS "memory;component;unit;offline")
  endforeach()
  add_test(NAME dmb.integration.offline_demo COMMAND dumas-memory-benchmark --offline-demo)
  set_tests_properties(dmb.integration.offline_demo PROPERTIES
    LABELS "memory;integration;offline" PASS_REGULAR_EXPRESSION "offline_component")
endif()
