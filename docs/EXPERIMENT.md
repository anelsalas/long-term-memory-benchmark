# Experiment configuration and results

Terms are explained in the [glossary](GLOSSARY.md).

P01 provides C++17 types in `long_term_memory_benchmark/experiment.hpp` and
`sha256(std::string_view)` in `hash.hpp`. Link `LongTermMemoryBenchmark::experiment`.

## Manifest version 1

| Field | Meaning |
| --- | --- |
| source_revision | Benchmark source revision |
| dataset | Name, revision and lowercase SHA256 of dataset bytes |
| case_ids | Unique ordered case selection |
| method, method_revision, method_parameters | Memory method and complete declared settings |
| reader, judge | Provider, model ID and version for each role |
| prompt_sha256 | Named checksums of the prompts/configuration material |
| context_token_limit, output_token_limit | Positive token budgets |
| concurrency | Positive simultaneous-case limit |
| run_parameters | Complete additional run settings, such as seed and replicate |

Parameter objects map names to strings. Encode a declared setting consistently;
include every setting that can change experiment behavior. Record credential
references as environment-variable names and keep credential values in protected
storage. Prompt and dataset checksums use exact bytes and preserve provenance.

`manifest_to_json` validates and emits compact UTF-8 JSON with sorted object keys.
`manifest_from_json` accepts key-order/formatting variations and validates the
same strict schema. Unknown fields, duplicate object keys, invalid types, duplicate
case IDs, incomplete identities and unsupported versions raise `invalid_argument`.
The canonical profile preserves case-array order and string values. Configuration
identity is SHA256 over `manifest_to_json` bytes; whitespace outside strings and
object key order are canonicalized. Unicode content remains byte-specific.

The SHA256 implementation is original portable C++17 following the algorithm in
[FIPS 180-4](https://csrc.nist.gov/pubs/fips/180-4/upd1/final). Known-answer tests
cover empty input, `abc`, a two-block vector and one million `a` bytes.

## Case records and coverage

`CaseRecord` carries case ID, lifecycle status, answer, error, provider request ID
and an optional boolean grade. Statuses are queued, running, completed, failed
and uncertain. A failed record requires an error; completed records keep their
answer separate from errors. Grades apply to completed records. Transition and
retry orchestration belongs to the provider lifecycle.

`summarize_cases` accounts for every manifest case: expected, present, missing,
each lifecycle count, graded, correct and completed-ungraded. Duplicate and unknown
record IDs are rejected. `known_correct_fraction_of_expected` divides known correct
answers by all expected cases. `graded_accuracy` describes the graded completed
subset and is null when that subset is empty. Interpret both with coverage;
ungraded, failed and absent cases remain visible.

`results_to_json(manifest, records)` writes schema version, configuration hash,
records in manifest case order and the computed coverage summary.
`results_from_json` verifies configuration identity, strict record types and the
summary against its own recomputation. An all-missing result is valid and explicit.

## Validation

Run `ltm_experiment_tests` directly or select CTest `ltm.unit.experiments`.
The target tests canonical identity, every declared field's effect, strict inputs,
SHA256 vectors, lifecycle coverage, escaped answers and result/hash round trips.
