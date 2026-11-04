# P06 command implementation checkpoint

Terms are explained in the [glossary](../docs/GLOSSARY.md).

October 31, 2026. Native Windows ARM64 command and injected HTTP validation.

Implemented the `ltm-compare` executable with dry-run planning and explicit execute
mode. It combines P01 configuration/results, P02 public-case loading, P03 shared
budget lifecycle, P04 public judge templates and P05 deterministic fresh-case gold.
Full-history/retrieved arms use the same fixed reader. A WinHTTP chat-completions
adapter uses configurable endpoint/model versions and injected transport for tests.

Observed behavioral TDD: six initial HTTP groups compiled against stubs, five
failed; implementation made all six pass. Covered payload model/output caps,
endpoint rejection, answer/usage parsing, sanitized failures, model mismatch,
incomplete/malformed responses and cancellation. No real HTTP model calls ran.

Observed native Release build and credential-free command execution:

- Fresh-only sample: two planned reader requests, zero provider requests and null
  model accuracy, with full/retrieved candidate prompts saved under a unique run.
- Public-plus-fresh sample: six planned requests, zero provider requests and null
  model accuracy, including the actual provenance-checked public selected case.
- Zero-budget execute: exit 2 with `execute_budget_gate`, before credential lookup
  or run artifact creation.
- HTTP transport tests pass on native ARM64. Compiler header-conversion warning
  was corrected using explicit conversion after ASCII validation.

Actual model access, exact response-version matching, tokenizer/framing bounds,
pricing and live measured acceptance remain G01/P06 verification. Official source
retrieval scores remain explicitly unavailable until the full user-only ranking
adapter is connected. This checkpoint is working command/planning evidence,
separate from measured model performance.

Files owned: http_provider.hpp/http_provider.cpp, compare_main.cpp,
http_provider_tests.cpp, docs/COMPARE.md and this report. Coordinator owns build
integration and publication; independent CLI checks are reported separately.
No credentials were read, model requests dispatched or background service started
by the worker's observed tests. No commits were made by this assignment.
