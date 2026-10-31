# Native foundation report

October 31, 2026. Agent implementation and observed test evidence.

Implemented a C++17 `MemoryPort`, a deterministic lexical reference, an offline
JSON-reporting CLI and behavioral CTest cases. All fixture people and events are
fictional public development data. The lexical method ranks shared unique ASCII
words, newest timestamp and source ID. It preserves source text and corrections,
filters person/time and validates UTC dates and duplicate source identities.

## Build and use

```powershell
cmake -S . -B build-arm64 -G "Visual Studio 17 2022" -A ARM64
cmake --build build-arm64 --config Release
ctest --test-dir build-arm64 -C Release --output-on-failure
.\build-arm64\Release\dumas-memory-benchmark.exe --offline-demo
```

Observed native tools: Visual Studio 2022 BuildTools, MSVC 19.44.35229.0,
Hostarm64/arm64 compiler, Windows SDK 10.0.22621.0. CMake/CTest binaries are under
`Common7/IDE/CommonExtensions/Microsoft/CMake/CMake/bin` within that installation.
An independent PE-header check confirmed ARM64 machine type `0xAA64`.

## Test evidence

- Red: compiled stub implementations; CTest observed 11 failures out of 12.
- Green: implemented retrieval/report behavior; Debug CTest observed 12/12 pass.
- Extended validation: Release CTest observed 15/15 pass, including date boundaries,
  source-recall denominator, JSON escaping and invalid timing rejection.
- Independent Python JSON parsing verified CLI report fields, source identities,
  null model measurements and nonnegative retrieval timing.

Targets: `dumas_memory_benchmark`, alias `DumasMemoryBenchmark::core`, CLI
`dumas-memory-benchmark`, and `dmb_component_tests`. Include
`dumas_memory_benchmark/memory.hpp` and `report.hpp`. CMake tests default on for
standalone builds and off under `add_subdirectory`; the integration option is
`DUMAS_MEMORY_BENCHMARK_BUILD_TESTS`. CTest labels are memory, component/unit or
integration, and offline. The parent build controls its own test registration.

The emitted source-recall fraction describes this fixture, separately from the
official LongMemEval any/all source metrics. The run measures offline component
behavior. Published dataset loading, model adapters, official scoring, case
checkpoints and budget enforcement are subsequent implementation milestones.

Files changed: CMakeLists.txt; include/dumas_memory_benchmark/; src/; tests/;
examples/offline-demo.md; this report. Builds are ignored under build-arm64/.
No process started by this task remains running. Coordinator owns publication.
