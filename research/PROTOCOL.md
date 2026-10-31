# Memory evaluation protocol

Measure which memory methods deliver correct, supported answers with useful
latency and affordable computation on everyday computers. Use a native C++17
ARM64 runner and versioned adapters for memory, inference and scoring.

## First experiments

| Priority | Experiment | Evidence gained | Setup effort |
| --- | --- | --- | --- |
| 1 | LongMemEval-S: full history versus retrieved evidence | Answer accuracy, source retrieval, speed and context usage | Moderate |
| 2 | Fresh fictional facts: no history, retrieved memory, direct evidence | Evidence dependence, corrections, identity and abstention | Moderate |
| 3 | LoCoMo: full context versus one published RAG configuration | Generalization to a second published workload | Lower |
| 4 | LongMemEval-M retrieval and temporal-filter ablations | Retrieval granularity, indexing and date-filter effects | Higher |
| 5 | Mem0 and Graphiti compared independently | Benefit and cost of derived facts and graph structure | Higher |

Match each reproduction to its paper, dataset revision and configuration in
[SOURCES.md](SOURCES.md). Begin with a fixed development subset and two arms;
expand to the complete declared evaluation after validating the execution path.
The 500-question LongMemEval-S evaluation is the primary shared workload.

## Execution and integration

The Surface runs experiment control, isolated memory storage, local retrieval,
measurement and reports. Inference adapters select the declared local endpoint
or remote service. Record model inference and network time separately where
observable. Dumas integrates a pinned benchmark version through its memory adapter.

Use deterministic component tests for ordinary code changes, a small integration
suite for memory changes, and explicitly triggered full model experiments with
declared budgets. Develop the benchmark through CMake/CTest and behavioral TDD.

## Frozen comparison

Record source revisions, dataset checksum, question IDs, all prompts, preprocessing,
model versions, context/output limits, sampling settings, concurrency, regions,
cache state, scorer version and budget before a run. Store credentials separately.

Ingest each history chronologically into an isolated instance. Keep evaluated
answers out of later ingestion. Give both arms the same authorized history,
question, answering model and scoring rules. Use matched context budgets for
engineering comparisons and original settings for historical reproduction.
Report full-history truncation explicitly.

Keep gold answers and source annotations in the evaluator. Pass original source
text and opaque source references to the evaluated system. Verify serialized
requests and annotation removal with deterministic tests.

Persist case/request IDs and queued, running, completed, failed or uncertain
states. Resume against the complete configuration hash. Report all requested
cases, failure counts, accuracy over valid responses and success over the full
requested workload. Preserve the original responses for audit.

## Measurements

| Measure | Required record |
| --- | --- |
| Answer correctness | Official score, category, requested/valid/error counts |
| Evidence retrieval | Source IDs/ranks, official any/all-source recall and denominator |
| Speed | Ingestion, retrieval and answer durations; median/p95; cold/warm conditions |
| Resources | Stage CPU time, sampled/lifetime peak memory, index/storage growth and background load; collector scope |
| Tokens and cost | Extraction, embedding, answering, grading and retry usage |

Preserve official turn/session mapping and retrieval exclusions. Record unavailable
source mappings and hosted usage as unavailable, with any estimates identified.
Keep cached/replayed results distinct from fresh latency and stochastic samples.

## Fresh facts and contamination controls

Generate fictional event ledgers with independently assigned values and compute
expected answers from those ledgers. Render conversations, validate dependent
dates/corrections and manually review a sample. Add distractor conversations to
test retrieval as history grows. Evaluate only records available at each checkpoint.

Create paired histories with changed facts and corresponding answers. Compare
the same model with no history, system-retrieved memory and direct source evidence.
Hold the question, date and instructions constant. Add evidence-removal cases for
abstention. Independent requests and namespaces isolate every condition.

Split development and final evaluation by story/fact family. Store hidden final
histories, gold and seeds outside the public repository. Publish completed test
versions with provenance and refresh hidden cases for subsequent studies.
Public scores and black-box probes support bounded conclusions about exposure;
fresh-fact evidence dependence supplies a separate generalization measurement.

## Analysis and publication

Report paired differences with uncertainty clustered by shared history/fact family.
Audit a blinded human sample and judge disagreements. Preserve failed cases,
category regressions and the number of tested variants. Use ablations to separate
retrieval, temporal fields, extraction, graphs and extra context.

For every adaptation record the original, replacement, reason, likely effect and
measured evidence. Separate historical-reference scores from controlled adapted
comparisons. Publish permitted code, configurations, reviewed results and a
reproduction command. Current experiment status is maintained in [STATUS.md](../STATUS.md).
