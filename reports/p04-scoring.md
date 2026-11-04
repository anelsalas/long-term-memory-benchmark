# P04 scoring evidence

Terms are explained in the [glossary](../docs/GLOSSARY.md).

October 31, 2026. Native Windows ARM64 Release implementation and offline parity.

## Delivered

`scoring.hpp`, `scoring.cpp`, `scoring_tests.cpp` and `docs/SCORING.md` provide
original-compatible retrieval definitions, complete aggregate accounting and
isolated QA grading requests. Standard C++17 interfaces use the P02 evaluator
sidecar; the coordinator supplies CMake integration. Adapted formulas/templates
retain the complete upstream MIT notice in source.

## Source inspection

Read `src/retrieval/eval_utils.py`, corpus construction/scoring/aggregation in
`src/retrieval/run_retrieval.py`, `src/evaluation/evaluate_qa.py` and `LICENSE`
at LongMemEval revision `9e0b455f4ef0e2ab8f2e582289761153549043fc`.

Preserved answer-marker/noans labels, user-only turn corpus, any/all Boolean
recall, original DCG weights, repeated-entry nDCG and unique-session effective-k.
Preserved abstention/no-user-target exclusions with explicit expected/missing
accounting. Restored original task templates, not newer permissive alternatives.

## Observed checks

- Red: ARM64 executable with P04 stubs failed all six initial behavior groups.
- Green: eight final behavior groups pass natively, covering metrics, session
  mapping, exclusions, bad rankings/mappings, prompts, verdicts and aggregates.
- All 540 permutations/cutoffs/annotation combinations emitted by the native
  parity harness match an independent Python standard-library implementation
  of the inspected formulas to absolute tolerance 1e-12.
- Twelve native QA prompts match the actual pinned upstream Python
  `get_anscheck_prompt` function byte-for-byte. The function was extracted with
  Python AST and executed independently of its imports and transport.
- SHA-256 instance hashes match Python hashlib. All five template hashes are
  frozen in the native unit tests. Scoped CTest `ltm.unit.scoring` passes.

NumPy is absent in the local Python environment. Retrieval numerical parity uses
the independent standard-library reference rather than executing the upstream
NumPy utilities. The exact upstream prompt function did execute. Arbitrary model
outputs and live judge behavior remain to evaluate through the provider stage.

## Explicit adaptations and compatibility

The rank input requires a full valid permutation; malformed and incomplete inputs
are errors. QA prompt parity currently requires string gold. Strict yes/no verdict
parsing marks ambiguous outputs invalid instead of upstream substring acceptance.
Missing cases and zero eligible denominators remain explicit. These choices are
documented in the adapter guide and must be recorded in experiment configuration.

Next: coordinator integration and independent suite execution; connect isolated
grade requests to the bounded provider lifecycle, then add a real-case metric
smoke and declared live judge configuration. This task made no model requests,
installed no packages, used no personal histories and left no running commands.
