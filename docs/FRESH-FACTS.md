# Fresh fictional fact controls

Terms are explained in the [glossary](GLOSSARY.md).

P05 builds consistent school-score timelines from structured event records.
Expected answers come from the ledger, independently of conversation rendering
or a language model. Native tests use original fictional records.

Include `long_term_memory_benchmark/fresh_facts.hpp`. The intended library target
is `LongTermMemoryBenchmark::fresh_facts`, linked to the dataset module for the
shared `CandidateCase` interface and input serializer.

```cpp
auto ledger = ltm::generate_development_ledger(42, 3, 7);
ltm::FactQuery question{"Lina-1", "astronomy", "2026-03-13T10:00:00Z"};
auto original = ltm::render_fresh_fact_case(ledger, question);
auto changed = ltm::render_fresh_fact_case(ltm::changed_fact_ledger(ledger), question);
auto input = ltm::candidate_input_json(original.candidate);
```

## Event semantics

`FactEvent` records source ID, fictional person/topic, UTC recording time, score
and an optional correction reference. Scores range from 0 to 100. Ledgers are
chronological with unique source IDs. A correction references the previous state
of the same person/topic and has a strictly later recording time. Explicit chains
retain earlier records for historical questions.

`recorded_at` governs information availability. Queries resolve the latest matching
person/topic record available at `as_of`. Missing information yields an empty
optional score and no supporting source. This first schema models recorded score
updates; separate event-valid-time semantics can enter a later version.

`resolve_fact` reads only the structured ledger and returns the expected value and
source. `render_fresh_fact_case` renders history up to the query checkpoint and
translates supporting sources to opaque runtime IDs. The result separates its
candidate object from expected answers and original-to-runtime source mappings.
Send the candidate object to inference; use expected/mapping fields in evaluation.
Case identity hashes the question identity, independently of ledger values.

Correction text is rebuilt from predecessor/current values. Changed-fact pairs
transform every score consistently while preserving identities, dates and correction
links. The question and case identity stay fixed. `remove_fact_history` removes an
entire person/topic chain, making that fact unavailable while preserving other
evidence. Removing a final correction alone would instead restore an earlier
visible score, so controls should declare their intended information state.

## Development and hidden evaluation

The deterministic public generator supports 1 to 128 fact families and up to
10,000 distractor records. Each family has an initial score, another person's
same-topic score and a correction. Distractors use other people/topics. The fixed
integer sampler supports reproducible development seeds across builds. Ledgers
accept at most 20,000 events and labels of 1 to 128 controlled ASCII characters.

Public development seeds and templates support component checks and review. Final
evaluation uses separately prepared private ledgers, independently sampled facts
and hidden seeds stored outside public source control. Validate and freeze stories
before evaluation; publish completed versions with provenance afterward. This API
performs in-memory construction and rendering; its caller owns private storage.

Use the same question/date under no-history, retrieved-memory and direct-evidence
conditions. Compare changed-fact pairs and evidence-removal cases. Score factual
values and source attribution separately. These controls measure evidence
dependence; public benchmark comparisons and broader story diversity supply
additional generalization evidence.

## Verification

`ltm_fresh_facts_tests` runs all behavior groups with no arguments. Tests cover
latest/historical answers, person separation, missing information, deterministic
generation, coherent changed facts, opaque sources, future-record isolation,
multi-correction chains, empty histories and malformed ledger rejection.
See [P05 evidence](../reports/p05-fresh-facts.md) for observed build/test results.
