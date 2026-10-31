# Current progress

Updated October 31, 2026. [Task board](TASKS.md).

## Verified

- Native C++17 ARM64 core, lexical reference, source/date/person filters and CPU/RAM reports.
- P01: versioned manifests, canonical configuration SHA-256, case lifecycle records and complete coverage summaries.
- P02: provenance-checked LongMemEval selected-case loading, opaque runtime IDs and separate evaluator gold/mappings.
- Independent Release ARM64 verification: 22/22 CTest checks passed.
- Extracted public-schema case validated: 53 sessions and 550 messages; dialogue stays in ignored runtime storage.
- Research protocol, paper citations, integration guide and model-independent [research brief](research/RESEARCH-OWNER.md).

The current runnable scope is offline retrieval, dataset preparation and experiment
accounting. Model-backed comparisons use the next provider/scoring adapters.

## Next

P03 bounded provider lifecycle and P04 official scoring are ready in parallel.
P05 fresh-fact generation prepares the contamination-control cases. G01 selects
model access and a pilot budget before live inference. P06 then compares one
published question and one fresh case under fixed conditions.

[Resource definitions](docs/RESOURCES.md) · [Schema](docs/EXPERIMENT.md) ·
[Dataset adapter](docs/LONGMEMEVAL.md)
