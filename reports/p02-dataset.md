# P02 dataset adapter evidence

Terms are explained in the [glossary](../docs/GLOSSARY.md).

October 31, 2026. Native Windows ARM64 Release validation.

## Delivered

- Standard C++17 candidate/evaluator/provenance API in `longmemeval.hpp`.
- Strict selected-case JSON loading, exact-byte SHA-256 verification and opaque
  runtime identities in `src/longmemeval.cpp`.
- Original fictional fixture, behavioral tests and adapter guide.

The coordinator supplies CMake integration and the pinned nlohmann dependency.
P01 supplies the shared SHA-256 implementation. This task changed its assigned
header/source/tests/fixture/doc/report paths only.

## Observed TDD and checks

- Red: ARM64 test executable compiled with the intentional P02 stub; all initial
  six behavior groups failed with `P02 TDD stub`.
- Green: implemented adapter; all seven final groups passed in native ARM64
  Release: schema, gold isolation, identity, abstention, malformed input,
  chronology and ISO dates.
- Gold isolation changes answers, gold source lists, question types, annotations,
  original question/source IDs and abstention suffix; runtime serialization stays
  identical when the question/history content and order are held fixed.
- Native validation against the coordinator's ignored extracted public case passed:
  513,118 bytes, 53 sessions, 550 messages. Printed metadata/counts only.

The extracted singleton identifies the cleaned LongMemEval-S source at Hugging
Face revision `98d7416c24c778c2fee6e6f3006e7a073259d48f`, with exact subset SHA-256
`04b8f255dd1f321553f35069dd4ef33d3036a94cf70ef020f73eb1490c812737`.
It is an extracted, reserialized case, rather than a whole-dataset checksum.
Its bytes and provenance remain in ignored runtime storage; the public fixture
is original fictional development data.

## Reproduce

Build target `ltm_longmemeval_tests`, then run the executable with no arguments
or use CTest `ltm.unit.longmemeval`. The optional `--validate-file` mode accepts
external case/provenance files and emits counts. See `docs/LONGMEMEVAL.md`.

## Next integration

Feed only the candidate object into memory/provider adapters. Join resulting
opaque source IDs against the evaluator sidecar for official turn/session scoring.
Record exact-byte dataset identity in P01 manifests. The bounded in-memory loader
supports initial subsets; larger-data throughput and memory remain to measure.
Official grader parity, live inference and benchmark scores are subsequent tasks.

No model calls, credentials, paid compute or published dialogue were introduced.
No task command remains running. Coordinator owns final review and publication.
