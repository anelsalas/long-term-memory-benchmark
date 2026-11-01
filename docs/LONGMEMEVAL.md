# LongMemEval dataset adapter

Load a selected published-schema case with byte-level identity and separate
candidate/evaluator data. Link `LongTermMemoryBenchmark::dataset` and include
`long_term_memory_benchmark/longmemeval.hpp`.

```cpp
ltm::DatasetProvenance source{uri, revision, expected_sha256};
auto loaded = ltm::load_longmemeval_case(bytes, question_id, source);
auto runtime_input = ltm::candidate_input_json(loaded.candidate);
```

`source_uri`, revision and a lowercase 64-character SHA-256 identify the exact
input bytes. A reserialized subset has its own checksum and should record its
extraction in provenance. `actual_sha256` records verified identity. The adapter
uses the shared original `ltm::sha256` implementation.

## Candidate and evaluator boundary

Candidate input contains an opaque case ID, question/date and chronological
sessions/messages with opaque source IDs, roles and source text. Serialization
uses an explicit whitelist. Original IDs can encode answer hints; the loader
replaces them with position-derived IDs before candidate input is formed.

The evaluator sidecar retains original question ID/type, abstention suffix,
original session mappings, one-based message positions, `has_answer` flags,
message annotations and gold session IDs. `answer_json` preserves the gold's JSON
value for a later official scorer. Candidate code should receive only the
candidate struct or its serialization. Gold annotations remain evaluator inputs.

History dates and question dates are preserved exactly. The adapter accepts the
published `YYYY/MM/DD (Day) HH:MM` format and UTC `YYYY-MM-DDTHH:MM:SSZ`, validates
calendar/time ranges and recognized weekday labels, and compares chronological
date components. The loader stably sorts sessions by date while preserving source occurrence IDs and original session mappings. Repeated original IDs remain separate occurrences. Sessions dated after the question are rejected and counted as an explicit evaluation adaptation. The weekday label remains source text rather than a recomputed
calendar constraint. Session timestamps describe recorded chat sessions; event
validity dates embedded in conversation text remain distinct.

## Validation and size

The root is a nonempty JSON array with unique question IDs. Full schema checks
apply to the selected case: required arrays/fields/types, matching session/date/ID
lengths, unique opaque occurrence IDs, history dates through question time, valid
user/assistant roles, Boolean answer annotations and known gold source references.
Gold answers preserve any non-null JSON value. Missing/unknown selections, duplicate
JSON keys, malformed dates and mismatched checksums raise `std::invalid_argument`.
Additional message annotations remain in the sidecar.

This initial adapter parses the input in memory. Its default byte limit is
256 MiB, configurable up to a hard 512 MiB; JSON objects and copies require extra
RAM. Use a pinned selected-case/subset artifact for initial runs, and measure
memory before larger workloads. A later streaming loader can retain this API.

## Checks

CTest `ltm.unit.longmemeval` runs the original fictional development fixture and
seven behavior groups. Gold/type/original-ID mutations verify identical candidate
serialization. Tests cover provenance, IDs, chronology, abstention, roles, dates,
duplicate JSON keys and expected failure paths.

An optional adapter smoke accepts separately stored public-case bytes and metadata:

```sh
ltm_longmemeval_tests --validate-file case.json case.provenance.json
```

Metadata fields are `source_uri`, `revision`, `sha256` and `case_id`. Output reports
counts and validation status. Schema validation establishes adapter compatibility;
answering and official metric parity are subsequent evaluation stages.

Format references: [LongMemEval dataset schema](https://github.com/xiaowu0162/LongMemEval/tree/9e0b455f4ef0e2ab8f2e582289761153549043fc#dataset-format)
and [paper](https://arxiv.org/abs/2410.10813v2). Record S/M, cleaned/original data
revision and extraction details in the experiment manifest.

## Repeated-ID scorer parity

The synthetic repeated-ID score matrix can be compared with the authors'
`src/retrieval/eval_utils.py` from revision `9e0b455f4ef0e2ab8f2e582289761153549043fc`:

```powershell
python scripts/check-duplicate-parity.py PATH_TO_UPSTREAM_EVAL_UTILS build-local-ui/Release/ltm_scoring_tests.exe
```

The check calls the upstream retrieval functions with a standard-library
replacement for NumPy DCG. It covered 162 ranking/cutoff combinations in the
November 1 run.
