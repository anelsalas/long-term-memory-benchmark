# P01 and P02 integration verification

Terms are explained in the [glossary](../docs/GLOSSARY.md).

October 31, 2026. Coordinator independently reviewed both modules and built their
combined Release configuration with native MSVC ARM64. CTest passed 22/22 checks:
the existing 19 checks, P01's nine-group suite, P02's seven-group suite and a
dataset-to-manifest/result identity integration test.

The integration test loads the original fictional fixture, forms a manifest from
the sanitized candidate and exact-byte dataset hash, round-trips queued results,
and rejects prior results after a configuration change. Gold and source annotations
remain in evaluator data. SHA-256 vectors and changed-field checks are recorded
in the P01 report; counterfactual metadata isolation is recorded in P02.

Independently validated the pinned extracted public case through the native adapter:
513,118 bytes, 53 sessions, 550 messages. Only counts were printed. The fixture
source revision and subset hash are recorded in P02's report; raw dialogue is
kept in ignored runtime storage. This is schema validation, not answer accuracy.

The restricted compiler launch stalled before compilation. Stopped only its
verified owned process tree and reran with the approved native compiler context;
all checks then completed. No model inference or compute purchase occurred.

Next: P03 bounded provider requests, P04 official scoring and P05 fresh facts.
