cmake_minimum_required(VERSION 3.20)
project(LongTermMemoryBenchmark VERSION 0.1.0 LANGUAGES CXX)

set(ltm_top_level OFF)
if(CMAKE_SOURCE_DIR STREQUAL PROJECT_SOURCE_DIR)
  set(ltm_top_level ON)
endif()
option(LONG_TERM_MEMORY_BENCHMARK_BUILD_TESTS "Build memory component tests" ${ltm_top_level})

add_library(long_term_memory_benchmark STATIC src/memory.cpp src/report.cpp src/resources.cpp)
add_library(LongTermMemoryBenchmark::core ALIAS long_term_memory_benchmark)
target_compile_features(long_term_memory_benchmark PUBLIC cxx_std_17)
if(WIN32)
  target_link_libraries(long_term_memory_benchmark PRIVATE psapi)
endif()
target_include_directories(long_term_memory_benchmark PUBLIC
  $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>)
if(MSVC)
  target_compile_options(long_term_memory_benchmark PRIVATE /W4 /permissive-)
else()
  target_compile_options(long_term_memory_benchmark PRIVATE -Wall -Wextra -Wpedantic)
endif()

add_executable(long-term-memory-benchmark src/main.cpp)
target_link_libraries(long-term-memory-benchmark PRIVATE LongTermMemoryBenchmark::core)

file(SHA256 "${CMAKE_CURRENT_SOURCE_DIR}/third_party/nlohmann/json.hpp" ltm_json_hash)
if(NOT ltm_json_hash STREQUAL "9bea4c8066ef4a1c206b2be5a36302f8926f7fdc6087af5d20b417d0cf103ea6")
  message(FATAL_ERROR "Vendored JSON header checksum differs from the pinned release")
endif()
add_library(ltm_experiments STATIC src/hash.cpp src/experiment.cpp)
add_library(LongTermMemoryBenchmark::experiment ALIAS ltm_experiments)
target_compile_features(ltm_experiments PUBLIC cxx_std_17)
target_include_directories(ltm_experiments PUBLIC "${CMAKE_CURRENT_SOURCE_DIR}/include")
target_include_directories(ltm_experiments SYSTEM PRIVATE "${CMAKE_CURRENT_SOURCE_DIR}/third_party")

add_library(ltm_dataset STATIC src/longmemeval.cpp)
add_library(LongTermMemoryBenchmark::dataset ALIAS ltm_dataset)
target_compile_features(ltm_dataset PUBLIC cxx_std_17)
target_link_libraries(ltm_dataset PUBLIC LongTermMemoryBenchmark::experiment)
target_include_directories(ltm_dataset SYSTEM PRIVATE "${CMAKE_CURRENT_SOURCE_DIR}/third_party")

add_library(ltm_provider STATIC src/provider.cpp)
add_library(LongTermMemoryBenchmark::provider ALIAS ltm_provider)
target_link_libraries(ltm_provider PUBLIC LongTermMemoryBenchmark::experiment)
target_include_directories(ltm_provider SYSTEM PRIVATE "${CMAKE_CURRENT_SOURCE_DIR}/third_party")
add_library(ltm_scoring STATIC src/scoring.cpp)
add_library(LongTermMemoryBenchmark::scoring ALIAS ltm_scoring)
target_link_libraries(ltm_scoring PUBLIC LongTermMemoryBenchmark::dataset)
target_include_directories(ltm_scoring SYSTEM PRIVATE "${CMAKE_CURRENT_SOURCE_DIR}/third_party")

add_library(ltm_fresh_facts STATIC src/fresh_facts.cpp)
add_library(LongTermMemoryBenchmark::fresh_facts ALIAS ltm_fresh_facts)
target_link_libraries(ltm_fresh_facts PUBLIC LongTermMemoryBenchmark::dataset)

add_library(ltm_http_provider STATIC src/http_provider.cpp)
add_library(LongTermMemoryBenchmark::http_provider ALIAS ltm_http_provider)
target_link_libraries(ltm_http_provider PUBLIC LongTermMemoryBenchmark::provider PRIVATE winhttp)
target_include_directories(ltm_http_provider SYSTEM PRIVATE "${CMAKE_CURRENT_SOURCE_DIR}/third_party")

add_executable(ltm-compare src/compare_main.cpp)
target_link_libraries(ltm-compare PRIVATE LongTermMemoryBenchmark::core
  LongTermMemoryBenchmark::dataset LongTermMemoryBenchmark::provider
  LongTermMemoryBenchmark::scoring LongTermMemoryBenchmark::fresh_facts
  LongTermMemoryBenchmark::http_provider)
target_include_directories(ltm-compare SYSTEM PRIVATE "${CMAKE_CURRENT_SOURCE_DIR}/third_party")

if(LONG_TERM_MEMORY_BENCHMARK_BUILD_TESTS)
  enable_testing()
  add_executable(ltm_component_tests tests/component_tests.cpp)
  target_link_libraries(ltm_component_tests PRIVATE LongTermMemoryBenchmark::core)
  foreach(case IN ITEMS exact_source correction_order person_filter unknown_query
      top_k as_of duplicate_id empty_record punctuation score_tie invalid_timestamp
      report_escape report_fraction report_invalid_time resource_delta resource_unavailable
      resource_json resource_process)
    add_test(NAME ltm.unit.${case} COMMAND ltm_component_tests ${case})
    set_tests_properties(ltm.unit.${case} PROPERTIES LABELS "memory;component;unit;offline")
  endforeach()
  add_test(NAME ltm.integration.offline_demo COMMAND long-term-memory-benchmark --offline-demo)
  set_tests_properties(ltm.integration.offline_demo PROPERTIES
    LABELS "memory;integration;offline" PASS_REGULAR_EXPRESSION "offline_component")
  add_executable(ltm_experiment_tests tests/experiment_tests.cpp)
  target_link_libraries(ltm_experiment_tests PRIVATE LongTermMemoryBenchmark::experiment)
  add_test(NAME ltm.unit.experiments COMMAND ltm_experiment_tests)
  set_tests_properties(ltm.unit.experiments PROPERTIES LABELS "memory;component;unit;offline")
  add_executable(ltm_longmemeval_tests tests/longmemeval_tests.cpp)
  target_link_libraries(ltm_longmemeval_tests PRIVATE LongTermMemoryBenchmark::dataset)
  target_include_directories(ltm_longmemeval_tests SYSTEM PRIVATE "${CMAKE_CURRENT_SOURCE_DIR}/third_party")
  target_compile_definitions(ltm_longmemeval_tests PRIVATE
    LTM_FIXTURE_PATH="${CMAKE_CURRENT_SOURCE_DIR}/tests/fixtures/longmemeval_development.json")
  add_test(NAME ltm.unit.longmemeval COMMAND ltm_longmemeval_tests)
  set_tests_properties(ltm.unit.longmemeval PROPERTIES LABELS "memory;component;unit;offline")
  add_executable(ltm_schema_bridge_tests tests/schema_bridge_tests.cpp)
  target_link_libraries(ltm_schema_bridge_tests PRIVATE LongTermMemoryBenchmark::dataset)
  target_compile_definitions(ltm_schema_bridge_tests PRIVATE
    LTM_FIXTURE_PATH="${CMAKE_CURRENT_SOURCE_DIR}/tests/fixtures/longmemeval_development.json")
  add_test(NAME ltm.integration.dataset_manifest COMMAND ltm_schema_bridge_tests)
  set_tests_properties(ltm.integration.dataset_manifest PROPERTIES LABELS "memory;integration;offline")
  add_executable(ltm_provider_tests tests/provider_tests.cpp)
  target_link_libraries(ltm_provider_tests PRIVATE LongTermMemoryBenchmark::provider)
  add_test(NAME ltm.unit.provider COMMAND ltm_provider_tests)
  set_tests_properties(ltm.unit.provider PROPERTIES LABELS "memory;unit;component;offline")
  add_executable(ltm_scoring_tests tests/scoring_tests.cpp)
  target_link_libraries(ltm_scoring_tests PRIVATE LongTermMemoryBenchmark::scoring)
  target_include_directories(ltm_scoring_tests SYSTEM PRIVATE "${CMAKE_CURRENT_SOURCE_DIR}/third_party")
  add_test(NAME ltm.unit.scoring COMMAND ltm_scoring_tests)
  set_tests_properties(ltm.unit.scoring PROPERTIES LABELS "memory;unit;component;offline")
  add_executable(ltm_fresh_facts_tests tests/fresh_facts_tests.cpp)
  target_link_libraries(ltm_fresh_facts_tests PRIVATE LongTermMemoryBenchmark::fresh_facts)
  add_test(NAME ltm.unit.fresh_facts COMMAND ltm_fresh_facts_tests)
  set_tests_properties(ltm.unit.fresh_facts PROPERTIES LABELS "memory;unit;component;offline")
  add_executable(ltm_http_provider_tests tests/http_provider_tests.cpp)
  target_link_libraries(ltm_http_provider_tests PRIVATE LongTermMemoryBenchmark::http_provider)
  target_include_directories(ltm_http_provider_tests SYSTEM PRIVATE "${CMAKE_CURRENT_SOURCE_DIR}/third_party")
  add_test(NAME ltm.unit.http_provider COMMAND ltm_http_provider_tests)
  set_tests_properties(ltm.unit.http_provider PROPERTIES LABELS "memory;unit;component;offline")
  add_test(NAME ltm.integration.compare_cli COMMAND ${CMAKE_COMMAND}
    "-DLTM_COMPARE_EXE=$<TARGET_FILE:ltm-compare>"
    "-DLTM_SOURCE_DIR=${CMAKE_CURRENT_SOURCE_DIR}"
    "-DLTM_TEST_DIR=${CMAKE_CURRENT_BINARY_DIR}/cli-tests"
    -P "${CMAKE_CURRENT_SOURCE_DIR}/tests/compare_cli_checks.cmake")
  set_tests_properties(ltm.integration.compare_cli PROPERTIES LABELS "memory;integration;offline")
endif()
