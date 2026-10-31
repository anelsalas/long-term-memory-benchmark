# Decisions

## Public benchmark and Dumas integration

Maintain an independent public benchmark with a versioned CMake integration point.
Dumas supplies an adapter for its memory implementation. Separate build targets
allow memory and audio development to progress independently.

## Experimental evidence

Keep model, dataset, prompts, evidence access and scoring fixed within comparisons.
Record deviations from published methods and their measured or uncertain effects.
Use fresh private fact histories to assess evidence dependence alongside public
benchmark scores. Publish completed evaluation versions with provenance.

## Native foundation

Use an original deterministic lexical reference to validate source identity,
person/time filtering, correction evidence and reporting before adding model
adapters. Label fractional fixture source coverage separately from official
LongMemEval retrieval definitions. Model measurements enter reports when observed.

## Initial publication

Publish original C++ source under Apache-2.0, with primary-paper citations and
pinned upstream references for adaptation. Use Linux and Windows ARM64 CI for
component checks. Dumas consumes a reviewed commit through an optional CMake target.

## Independent project name

October 31, 2026. User requested independent branding. Use Long-Term Memory
Benchmark, repository `long-term-memory-benchmark`, CMake namespace
`LongTermMemoryBenchmark`, C++ namespace `ltm` and neutral executable/header names.
Historical reports retain the identifiers used by their recorded revisions.
