# P06 command acceptance checks

October 31, 2026. Native Windows ARM64 offline CLI acceptance.

Added `tests/compare_cli_checks.cmake`, using CMake 3.20 process and JSON APIs.
The coordinator registers it as `ltm.integration.compare_cli` with the command
path, source directory and an ignored build-test directory. Each invocation
creates a fresh test workspace and retains its artifacts for inspection.

## Verified against the implemented command

- Generated fresh-only configuration uses zero budgets, a unique nonexistent
  credential environment name and an `.invalid` HTTPS endpoint.
- Actual stdout identifies a dry-run plan, zero dispatched requests, two planned
  reader requests and null model accuracy. Fresh grading is deterministic, so
  planned model-judge requests are zero.
- Manifest/plan/evaluator files exist. Both full-history and retrieved-evidence
  conditions are present with the expected case IDs.
- Exact reader prompt hashes match actual prompt bytes. The JSON candidate is
  extracted structurally, including support for trailing output instructions.
- Candidate/session/message whitelists exclude evaluator fields; source IDs are
  opaque. Planned source IDs match the actual serialized reader evidence.
- Evaluator gold occupies a separate artifact. Dry runs produce planning
  artifacts rather than measured-result or summary files.
- Execute with zero budget returns the explicit `execute_budget_gate` error
  before credential lookup/output creation. Existing artifact directories remain
  unchanged, and the error identifies no artifact directory.
- Malformed configuration returns command-error status. Repeated dry runs create
  distinct run directories and preserve every earlier artifact checksum.
- An occupied output file is preserved and yields explicit failure.

## Observed integration

An initial test configuration failed the new answer-byte preflight because its
response cap was 4,096 while the command's default answer cap was 65,536.
The acceptance configuration now explicitly sets a 2,048-byte answer cap.
The command exposes safe stage markers to distinguish budget rejection from a
later credential failure. Test expectations follow the implemented deterministic
fresh grader and the command-error JSON contract.

Direct script execution passed against `build-compare/Release/ltm-compare.exe`.
Scoped CTest passed 1/1 against `build-review/Release/ltm-compare.exe`, including
the final source-membership checks. The test requires CMake and the native
executable; CI execution uses no Python dependency.

This validates command preparation, artifact integrity and pre-dispatch gates.
Live provider behavior and measured answer accuracy remain separate experiments.
No model calls, credentials, paid compute or public benchmark dialogue were used.
Only the new acceptance script and this report were edited by this assignment.
