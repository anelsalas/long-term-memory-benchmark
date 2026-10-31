# Application integration

Pin this repository to a reviewed commit. Add it as a CMake subdirectory and link
the adapter or test target to `LongTermMemoryBenchmark::core`.

```cmake
add_subdirectory("${MEMORY_BENCHMARK_SOURCE}" memory-benchmark EXCLUDE_FROM_ALL)
target_link_libraries(my_memory_adapter PRIVATE LongTermMemoryBenchmark::core)
```

Set `LONG_TERM_MEMORY_BENCHMARK_BUILD_TESTS=ON` before adding the subdirectory to
register the component suite. Enable testing in the parent project. CTest labels
select `unit`, `integration`, `offline` or `memory` checks.

Implement `ltm::MemoryPort` for a candidate's `store` and `search` operations.
Keep source IDs, person identity and timestamps attached to retrieved records.
Each experiment gets an isolated store and a versioned configuration.

Use `ctest -L unit` for fast changes and `ctest -L integration` for adapter checks.
Model experiments produce reviewed result artifacts under declared token/request
and spending limits. Publish their configurations and measured outcomes together.
