# P05 fresh-fact evidence

Terms are explained in the [glossary](../docs/GLOSSARY.md).

October 31, 2026. Native Windows ARM64 Release implementation.

## Delivered

- `fresh_facts.hpp` and `fresh_facts.cpp`: structured event ledger, bounded seeded
  development generator, validated correction chains, independent expected-answer
  resolver, changed-fact and evidence-removal transformations, runtime renderer.
- `fresh_facts_tests.cpp`: original fictional records and behavioral checks.
- `docs/FRESH-FACTS.md`: semantics, integration and hidden-evaluation boundary.

## Observed TDD

An isolated ignored `build-p05/harness` CMake project compiled the assigned source,
P02 serializer and shared SHA-256 while root integration was being prepared.
MSVC 19.44 configured with Visual Studio 2022 and `-A ARM64`.

- Red: the initial native executable failed all eight behavior groups against
  intentional P05 stubs.
- Green: all nine final groups passed after implementation and chain/identity
  extensions: corrections, historical answers, identities, generation, changed
  values, evidence removal, candidate boundary, correction chain, invalid ledgers.
- Known fixture expectations are independently specified: 83 before correction,
  91 afterward, a different person's score 72 and an absent chemistry score.
- Changed predecessor/current values regenerate consistent source text. Future
  records remain outside earlier checkpoint input. Opaque question identity
  depends on question fields rather than gold facts.

## Build wiring

Coordinator integration: library `ltm_fresh_facts` with alias
`LongTermMemoryBenchmark::fresh_facts`, source `src/fresh_facts.cpp`, public
dependency `LongTermMemoryBenchmark::dataset`; test executable
`ltm_fresh_facts_tests` from `tests/fresh_facts_tests.cpp`, running with no args.
The task-owned build is an isolated behavior harness; root CMake and test-board
updates belong to the coordinator.

## Scope and next action

This version uses controlled score/correction templates and current-as-recorded
time. It produces original public development histories; private final stories
and seed storage are caller-owned external inputs. Model calls, arbitrary free-text
grading and empirical contamination claims belong to later configured experiments.
Continue with paired no-history/retrieval/direct-evidence requests through the
provider layer and separate value/source scoring. Measure broader story diversity
and workload scaling under declared configurations.

P03/P04 files were preserved. This task accessed no credentials, introduced no
actual personal histories, made no paid calls and left no running command.
Coordinator owns integration, final independent validation and publication.
