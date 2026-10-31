# Long-Term Memory Benchmark

Measure and improve reliable long-term memory access on everyday computers.

[Pending tasks](TASKS.md) Â· [Current status](STATUS.md) Â· [Research protocol](research/PROTOCOL.md)

Compare memory methods by answer accuracy, evidence retrieval, latency, token
usage and cost. Build native C++17 components for Windows ARM64, with
CMake targets and automated tests that applications can consume as a pinned dependency.

## Run

The current executable runs synthetic lexical-retrieval component checks and
prints source coverage, timing, CPU time and process-memory samples as JSON.
The paired comparison command prepares fresh/published cases and executes
configured HTTPS model requests under declared budgets.

```powershell
cmake -S . -B build -G "Visual Studio 17 2022" -A ARM64
cmake --build build --config Release
ctest --test-dir build -C Release --output-on-failure
```

Run `build/Release/long-term-memory-benchmark --offline-demo`.

Library modules provide [experiment manifests and results](docs/EXPERIMENT.md)
and a [LongMemEval dataset adapter](docs/LONGMEMEVAL.md).

## Paired comparison

```powershell
build/Release/ltm-compare --config configs/pilot-fresh-plan.example.json --dry-run
```

Dry-run saves full-history/retrieved-evidence plans with zero model calls.
[Command and artifacts](docs/COMPARE.md) define dashboard integration and execute
configuration. [Model access](research/MODEL-ACCESS.md) records the current shortlist
and proposed pilot envelope. Example configs have zero spending allowances.

## Research foundation

- [LongMemEval: Benchmarking Chat Assistants on Long-Term Interactive Memory](https://arxiv.org/abs/2410.10813): memory evaluation and retrieval design.
- [Mem0: Building Production-Ready AI Agents with Scalable Long-Term Memory](https://arxiv.org/abs/2504.19413): fact extraction, updates and efficiency.
- [Zep: A Temporal Knowledge Graph Architecture for Agent Memory](https://arxiv.org/abs/2501.13956): temporal relationships and evidence.

[Review and continuation guide](REVIEW-START-HERE.md) summarizes implementation,
verified tests, limitations and resume steps for another model.

## Develop and evaluate

Use [the objective](PROJECT.md), [research protocol](research/PROTOCOL.md),
[source attribution](research/SOURCES.md), [Application integration](docs/INTEGRATION.md),
and [current progress](STATUS.md). Research ownership instructions are in
[research/RESEARCH-OWNER.md](research/RESEARCH-OWNER.md).

Fast component tests run on code changes. Selected integration tests exercise
memory adapters. Full model experiments use explicit configurations and budgets.
Public results identify dataset, model, method, scorer and execution versions.

[Resource measurements](docs/RESOURCES.md) define CPU time, sampled memory,
process-lifetime peak memory and stage timing for fair efficiency comparisons.

License: [Apache-2.0](LICENSE).
