# Current progress

Project name: Long-Term Memory Benchmark.
Repository: https://github.com/anelsalas/long-term-memory-benchmark
October 31 rename validation: native ARM64 Release CTest 15/15 passed.
Public CMake API: `LongTermMemoryBenchmark::core`; executable:
`long-term-memory-benchmark`; headers: `long_term_memory_benchmark/`.

October 31, 2026. Native C++17 foundation and research protocol completed.

- Coordinator independently built Release with native ARM64 MSVC 19.44.
- CTest: 15/15 checks passed (14 component tests and one offline CLI integration).
- Independent JSON parsing and PE-header checks verified report fields and ARM64.
- Two agent assignments delivered implementation and methodological review.
- GitHub CI targets Linux and Windows ARM64; results appear in Actions.

The executable reports synthetic lexical source retrieval and measured search
time. Model-backed accuracy, token/cost accounting and published benchmark
comparisons are the next implementation stage.

Next: dataset/model/scorer adapters for one fixed LongMemEval-S question and one
fresh-fact case, followed by a bounded two-arm pilot with request accounting.
Research ownership instructions: research/CLAUDE.md.
