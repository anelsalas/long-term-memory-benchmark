# Current progress

Updated October 31, 2026. [Task board](TASKS.md).

## Verified

Target platform: native Windows ARM64 with MSVC and CMake/CTest.

- Native C++17 ARM64 core, lexical reference, source/date/person filters and CPU/RAM reports.
- P01: versioned manifests, canonical configuration SHA-256, case lifecycle records and complete coverage summaries.
- P02: provenance-checked LongMemEval selected-case loading, opaque runtime IDs and separate evaluator gold/mappings.
- Independent Release ARM64 verification: 25/25 CTest checks passed.
- Extracted public-schema case validated: 53 sessions and 550 messages; dialogue stays in ignored runtime storage.
- Research protocol, paper citations, integration guide and model-independent [research brief](research/RESEARCH-OWNER.md).

P03 bounded provider lifecycle and P04 official-compatible scoring are implemented.
P05 seeded fictional event ledgers compute expected answers and coherent corrections.
The combined native Release suite passed 25/25 checks after integration.
Agent reports record observed behavioral TDD and scoped parity checks.

## Next

P03-P05 source and handoff are published on codex/memory-review at ebc0105;
core review and main integration are pending. The user has started Claude on
the separate codex/dashboard workstream. Coordinator focus is core review,
model-selection/access and the first measured comparison. G01 selects
model access and a pilot budget before live inference. P06 then compares one
published question and one fresh case under fixed conditions.

[Resource definitions](docs/RESOURCES.md) Â· [Schema](docs/EXPERIMENT.md) Â·
[Dataset adapter](docs/LONGMEMEVAL.md)
