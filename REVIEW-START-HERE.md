# Review and continuation guide

Terms are explained in the [glossary](docs/GLOSSARY.md).

October 31, 2026. Codex summary of observed work.

## Objective

Measure reliable, affordable long-term memory on an everyday Windows ARM64
computer. Compare answer accuracy, evidence, latency, CPU/RAM, tokens and cost.
Research foundation: LongMemEval, Mem0 and Zep; links are in README.md and
research/SOURCES.md. Keep published reproduction and fresh-fact evaluation as
separate tracks. Record substitutions and their limits.

## Read in order

1. AGENTS.md, PROJECT.md, STATUS.md and TASKS.md.
2. research/PROTOCOL.md, research/SOURCES.md and research/RESEARCH-OWNER.md.
3. docs/EXPERIMENT.md and docs/LONGMEMEVAL.md.
4. docs/PROVIDER.md, docs/SCORING.md and docs/FRESH-FACTS.md.
5. reports/p03-provider.md, reports/p04-scoring.md, reports/p05-fresh-facts.md.
6. The newest handoff under handoffs/.

## Implemented

- C++17 native ARM64 core, lexical reference, source/person/time filtering.
- Wall time, Windows process CPU and resident/private-commit memory reporting.
- P01 canonical experiment identity and complete expected/missing/failure accounting.
- P02 bounded provenance-checked LongMemEval adapter with candidate/evaluator isolation.
- P03 provider lifecycle: budget reservation/settlement, safe retries, cancellation,
  request provenance and explicit uncertain/fatal outcomes; injectable provider.
- P04 upstream-compatible retrieval metrics, isolated grading prompts, strict verdict
  parsing and documented adaptations. Agent recorded 540 reference comparisons
  and 12 byte-matched upstream prompts; inspect its report for exact scope.
- P05 deterministic public-development score/correction histories, structured gold,
  changed-fact pairs, evidence removal and as-of/person isolation.

## Observed verification

Coordinator built integrated MSVC native ARM64 Release in build-review and ran
CTest: 25/25 passed. Worker reports document behavioral red/green. This verifies
component/integration behavior. Empirical model-answer comparisons await G01/P06.
Agent-reported parity evidence should be independently rerun during review.

```powershell
cmake -S . -B build-review -G "Visual Studio 17 2022" -A ARM64
cmake --build build-review --config Release --parallel 2 -- /nodeReuse:false
ctest --test-dir build-review -C Release --output-on-failure
```

## Priority review

Review scoring against pinned upstream rules and declared deviations; candidate
inputs for gold/source-ID leakage; generator correction consistency and diversity;
provider accounting, uncertain submissions and cooperative timeout semantics.
P05 currently uses bounded score/correction templates. Expand story diversity
through explicit independently testable event types. Validate real provider
transport, token accounting and pricing before a live experiment.

## Work streams

Core integration/review: coordinator. Dashboard: native_runner agent, separate
codex/dashboard branch. Research agent completed P04/P05. Dashboard uses small
same-origin HTML/CSS/JavaScript assets: Control, Setup, Results, Credits. Its next
integration includes authenticated native API, setup validation and LAN/mobile
verification. Model selection belongs to coordinator and remains pending:
compare accessible original configurations and current substitutes, pin versions,
record terms/pricing and obtain a declared request/token/spend cap.

## Current publication boundary

Published main previously ends at f3f9e036385f24728656766685f571bccdaa9dfc.
P03-P05 are prepared as a review branch; consult Git refs for publication status.
Dumas optional consumer remains pinned to 8a7430326d941d80f637e86ea2973613a76078a5.
After review, publish a coherent core revision and verify the updated consumer.

## Data and operations

Downloaded histories stay in ignored runtime storage. Hidden final ledgers and
seeds stay in private external storage. Credentials stay in protected server-side
storage. This work has made zero paid model calls. Preserve Dumas voice edits,
provider settings, signing keys and application data. Historical records preserve
the platforms/names used at their recorded revisions; current target is Windows ARM64.
