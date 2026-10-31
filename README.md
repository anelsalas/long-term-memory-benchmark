# Dumas Memory Benchmark

Measure and improve reliable long-term memory access on everyday computers.

Compare memory methods by answer accuracy, evidence retrieval, latency, token
usage and cost. Build native C++17 components for Windows ARM64, with portable
CMake targets and automated tests that Dumas can consume as a pinned dependency.

## Run

The current executable runs synthetic lexical-retrieval component checks and
prints source coverage and timing as JSON. Model-backed evaluations are the next milestone.

```sh
cmake -S . -B build
cmake --build build --config Release
ctest --test-dir build -C Release --output-on-failure
```

On Windows ARM64, configure with `-G "Visual Studio 17 2022" -A ARM64`.
Run `build/Release/dumas-memory-benchmark --offline-demo` on Visual Studio builds,
or `build/dumas-memory-benchmark --offline-demo` on single-configuration builds.

## Research foundation

- [LongMemEval: Benchmarking Chat Assistants on Long-Term Interactive Memory](https://arxiv.org/abs/2410.10813): memory evaluation and retrieval design.
- [Mem0: Building Production-Ready AI Agents with Scalable Long-Term Memory](https://arxiv.org/abs/2504.19413): fact extraction, updates and efficiency.
- [Zep: A Temporal Knowledge Graph Architecture for Agent Memory](https://arxiv.org/abs/2501.13956): temporal relationships and evidence.

## Develop and evaluate

Use [the objective](PROJECT.md), [research protocol](research/PROTOCOL.md),
[source attribution](research/SOURCES.md), [Dumas integration](docs/INTEGRATION.md),
and [current progress](STATUS.md). Research ownership instructions are in
[research/CLAUDE.md](research/CLAUDE.md).

Fast component tests run on code changes. Selected integration tests exercise
memory adapters. Full model experiments use explicit configurations and budgets.
Public results identify dataset, model, method, scorer and execution versions.

License: [Apache-2.0](LICENSE).
