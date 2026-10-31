# Offline retrieval example

Run `long-term-memory-benchmark --offline-demo` to emit a JSON component report.

| Source | Person | Recorded UTC | Fictional event |
| --- | --- | --- | --- |
| school-01 | maya | 2026-03-03 10:00 | Science project score 83 |
| school-02 | maya | 2026-03-12 10:00 | Science project score corrected to 91 |
| school-03 | sam | 2026-03-13 10:00 | Science project score 72 |

The query `science project score`, filtered to `maya` with a two-record limit,
retrieves school-02 then school-01. Both records remain available as evidence.
The implementation ranks unique ASCII word overlap, then newest timestamp,
then source ID. Person filters and UTC `as_of` cutoffs apply before ranking.

The report identifies `offline_component` and `public_synthetic_development`.
`source_recall` is the fraction of unique expected IDs retrieved; an empty
expected set produces a null metric. This fixture uses two expected IDs, both
retrieved. Model answer accuracy and model tokens are null; model calls are zero.
The observed duration covers only this reference search on the supplied records.
