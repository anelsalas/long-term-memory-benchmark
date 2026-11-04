# P03-P05 correctness review

Terms are explained in the [glossary](../docs/GLOSSARY.md).

October 31, 2026. Bounded native Windows ARM64 review for core integration.
This worker previously implemented P04/P05; this pass re-examined those components
and reviewed P03 read-only. Coordinator verification supplies the integration check.

## Fixed development-data shortcut

P05 generated the other person's score as `(initial + 37) % 101` and every
correction as `(initial + 9) % 101`. Those fixed relationships let an evaluator
predict a person's updated fact from other visible facts without finding the
decisive record. This weakened the intended identity and missing-evidence controls.

Added `independent_generated_facts` across 64 public development seeds. Observed
red: the prior implementation failed because all person offsets were identical.
The updated generator draws the other person's value separately and draws a
variable nonzero correction offset. Corrections remain distinct from their
initial values, and source rendering/structured gold remain consistent.
Observed green: all ten P05 behavior groups pass natively.

The fixed sampler remains deterministic for a seed at a declared source revision.
The revised draw sequence changes generated values, so artifacts must retain the
generator source/configuration version. Public deterministic cases support
development checks; private final-evaluation facts still require independent
provenance and storage.

## Scorer and leakage checks

Rechecked the pinned LongMemEval corpus construction, any/all recall, original
nDCG weights, repeated-entry turn/session mapping, effective-k expansion and
abstention/no-user-target exclusions. The eligible denominator remains separate
from supplied, missing and excluded counts. Full-rank compatibility and string-gold
limitations remain explicit. Strict judge parsing is the recorded adaptation.

The freshly rebuilt reviewer binary passed eight scorer behavior groups and
540 ranking/cutoff/annotation combinations against the independent standard-library
reference. Twelve prompts again matched the pinned upstream Python function
byte-for-byte, with matching instance SHA-256. NumPy execution remains outside
this local parity check; the exact extracted upstream prompt function did execute.

Candidate serialization stays separate from evaluator gold, annotations and
original hint-bearing source IDs. Fresh candidate input contains only records
available at the requested checkpoint. Changed-fact rendering recomputes both
predecessor and corrected values. Whole-chain removal produces missing-information
cases; correction chains, person isolation and query identity checks pass.

## Provider read-only findings

Checked reservation/settlement, unknown-usage handling, request identity, response
normalization, uncertain submission handling and cooperative deadline metadata.
Scoped provider tests pass in the reviewer build.

The runner serializes dispatch under one mutex. Pilot concurrency should identify
that actual execution mode. Reader and judge identities can differ while the
policy has one rate pair (`src/provider.cpp:269`, settlement at `:299`). Coordinator
confirmed pilot policy uses conservative maximum rates for both roles under one
global cap. Accounted cost is therefore a declared bound/estimate, not necessarily
the provider's final invoice. Future role-specific pricing should retain that
distinction. Provider source was preserved in this review.

## Verification and integration

Root configuration briefly referenced transport files still being created by the
parallel worker. Used an ignored `build-correctness/harness` CMake project to build
the current P03/P04/P05 sources without editing coordinator build files.
MSVC ARM64 Release built successfully; CTest observed 3/3 suites passing:
provider, scoring and fresh facts. Scoped whitespace checks passed.

Merge changes: `src/fresh_facts.cpp`, `tests/fresh_facts_tests.cpp` and this report.
No scorer semantic changes were needed. New transport/CLI and dashboard work
remain separate review scopes. This pass made no model calls, read no credentials,
changed no root build files and left no running task command.
