# Native comparison command

`ltm-compare` prepares or executes a fixed-reader full-history versus retrieved-
evidence comparison on Windows ARM64. The command always includes one generated
fictional score case and optionally one provenance-checked LongMemEval case.

```powershell
cmake --build build --config Release --target ltm-compare
build/Release/ltm-compare --config configs/pilot-fresh-plan.example.json --dry-run
```

Use `--execute` with a reviewed runtime configuration, verified model access and
explicit budgets. `--dry-run` prepares artifacts and performs zero provider calls
or credential reads. The example configurations have zero execution budgets.
Exit codes: 0 = successful plan or fully graded execution, 2 = usage/configuration/
artifact error, 3 = measured execution with incomplete/failed/ungraded cases.
Stdout is one JSON summary; stderr uses sanitized `ltm_compare_error:<stage>` codes.

## Runtime configuration

Schema version 1 rejects unknown/duplicate object fields. Counts accept nonnegative
JSON integers or exact decimal strings. Files resolve relative to the config file.

- `source_revision`: reviewed benchmark revision.
- `endpoint_url`: full HTTPS chat-completions URL; `api_key_env`: credential
  environment-variable name. Credential values remain runtime-only.
- `reader` and `judge`: `{id, version}`. Version is the exact model sent on the wire
  and must match the returned response model. Families/aliases require explicit
  version verification for that provider before execution.
- `provider_policy`: P03 request/token/nanodollar budgets, maximum input/output
  rates covering both models, fixed request fee, prompt/response byte caps and
  maximum attempts. One ledger covers all reader and judge requests.
- `context_token_limit`, `input_token_upper_bound`, `output_token_limit`,
  `judge_output_token_limit`, `chat_framing_token_allowance` (default 256) and
  `tokenizer_bound_verified`. Prompt UTF-8 bytes plus framing must fit the input
  upper bound. Execution also requires the declared tokenizer-bound verification;
  confirm provider-specific framing and all billable thinking/output token rules.
- `output_token_parameter`: `max_tokens` or `max_completion_tokens`, selected for
  the declared model/API compatibility. `temperature` is a number or null; null
  omits that request field. The public judge uses temperature zero.
- `seed`, `top_k` (1–1000), `timeout_ms`, `output_directory` and optional
  `max_candidate_answer_bytes` (default 65536, within response byte cap).
- Optional `public_case`: `{path, question_id, source_uri, revision, sha256}` for
  a selected-case JSON file. Its exact bytes must match the declared digest.

Input configuration is bounded to 1 MiB; the dataset input to 256 MiB before
allocation. Judge templates and the declared maximum frozen-answer size are
checked before execution. The input-byte bound is conservative only under the
verified tokenizer/framing assumptions; model-specific verification is G01.

## Conditions and grading

The fresh case uses the independent P05 ledger and asks for an integer score.
Full-history and retrieved-evidence use the same reader, answer instruction and
declared limits. Exact integer matching supplies its primary deterministic grade.
The seeded public-development generator is a plumbing/behavior fixture; fresh
hidden evaluation stories remain a separate research step.

The retrieved arm uses the native ASCII lexical reference over both user and
assistant messages. Selection returns the configured top-k message sources;
selected messages retain original chronological presentation. This is an adapted
reference, distinct from the paper's Stella/user-only retrieval configuration.
Full history supplies all candidate records; oversized prompts fail preflight
rather than being silently truncated.

Public answers are saved before the official-compatible grading prompt is built.
The fixed judge then grades the frozen answer with P04's strict yes/no parser.
The original judge output limit is 10; any configured larger cap is recorded as
an adaptation in the manifest. Thinking models may need a different cap, requiring
the same declared condition in comparisons.

The command currently reports official retrieval metrics as null with a reason:
the P04 scorer requires a complete user-only corpus ranking, while this reference
currently supplies a selected both-role context. Exact supplied source IDs are
saved for evidence review. Full scorer/ranking integration is a subsequent step.

Fresh-only planning counts two reader requests; public-plus-fresh planning counts
four readers and two public judges. Retry limits can increase the attempt count
within the same global budget. Reported usage is settled; missing usage retains
its reservation. Failed or ungraded cases remain visible in P01 coverage.

## Transport and artifacts

The WinHTTP adapter sends a single non-streaming chat-completions request and
reads its response in bounded chunks. It uses normal TLS verification, HTTPS-only
URLs, disabled redirects, finite send/receive timeouts and cooperative cancellation.
429/5xx and ambiguous submissions produce uncertainty without blind retry. Errors
record HTTP categories while withholding raw error bodies. API response model,
request IDs, usage and fingerprint metadata are retained when available.

Each invocation creates a unique `run-*` subdirectory. Existing artifacts are
preserved. The directory contains:

- `manifest.json`, `plan.json` and separate `evaluator-gold.json`.
- In execution, per-case reader intent and frozen reader result, public judge
  intent, and combined result files with exact prompts, source IDs, API attempts,
  timings, reported usage and reservations.
- `results.json` with expected-case coverage and `summary.json`.

Resource records identify local-process scope and report reader CPU delta,
resident/private-commit samples and lifetime peak. Retrieval/preparation and reader
API wall time remain separate. API usage/cost describes the declared accounting
model; missing measurements retain null/unreported state. Credential occurrences
in artifact string values are redacted and marked. Keep runtime artifacts in
ignored protected storage and review them before any publication.

SIGINT requests cooperative cancellation. Provider calls use P03's deadline and
uncertain-submission contract. Re-running a command creates a new experiment;
durable resume/reconciliation is P07.

Sources: [Chat Completions create](https://developers.openai.com/api/reference/resources/chat/subresources/completions/methods/create),
[Gemini compatibility](https://ai.google.dev/gemini-api/docs/openai/),
[WinHTTP response reads](https://learn.microsoft.com/en-us/windows/win32/api/winhttp/nf-winhttp-winhttpreaddata),
[WinHTTP redirect policy](https://learn.microsoft.com/en-us/windows/win32/winhttp/option-flags).

## Local loopback profile

Set `execution_mode: "local"` for a numeric `127.0.0.1` or `[::1]` endpoint.
This explicit profile supports HTTP on the Surface, bypasses proxies, and omits
the bearer header when the credential is empty. Omit `api_key_env` or set it to
an empty string. Declare zero input/output/request prices and zero
`max_cost_nanos`; positive request/token caps and the verified token-bound
declaration still apply to execution. The manifest records the execution mode.
The default `https` profile retains external TLS, credential and spending gates.

The local payload contains `model`, `messages`, `stream`, the selected output cap,
and temperature only when configured. Set temperature to null for the existing
Qwen service, which accepts `max_tokens` up to32. Its model identifier is
`qwen3-0.6b-q8_0`; returned model identity must match exactly. Partial local token
usage remains in response metadata; combined input/output usage stays unavailable
and the request reservation remains accounted when input usage is absent.

Set `prompt_profile: "compact_local_smoke"` and `fresh_distractors: 0` for a tiny
generated correction fixture. This local-only profile renders dated source text
and the question directly, preserving selected evidence. It checks the service's
512 serialized context-byte limit including the user role framing before
dispatch. Both output limits should be32, input upper bound512, framing32 and
context limit1024. The manifest records this profile and distractor count.
This is a plumbing comparison with a tiny fixture. The generic fresh benchmark
retains its original history and prompt format. The local service reports
`finish_reason: stop` unconditionally; the manifest records that compatibility
limit so it supplies no evidence that generation finished naturally.

## Four evidence conditions

Set `conditions` to `no_memory`, `full_history`, `retrieved_evidence`, and
`known_evidence` in the desired order. Each receives the same answer instruction.
Known evidence uses the fresh ledger's supporting source IDs. Its role is to
measure answering with ideal evidence. The keyword method selects its own sources.
The default remains the two-condition full/retrieved comparison.

Run `scripts/run-qwen-controls.ps1` from the repository root after building
`build-vs2026/Release/ltm-compare.exe`. It freezes 20 correction histories
(seeds 42–61), rotates condition order, preflights every prompt, and executes
80 local requests. Each seed permits four requests, 4096 reserved tokens and $0.
Use a fresh output directory for each experiment. Then run
`scripts/report-qwen-controls.ps1 -RunDirectory <directory>` to produce the
reviewable JSON observations and dashboard Results page.

The compact service profile fits 512 context bytes and 32 output tokens.
Gold agreement for no-memory requests is a guessing diagnostic. Exact UNKNOWN
responses are recorded separately. Output-at-cap counts expose possible truncation.
Seed variations share one correction template; broader question types follow.

For the 500-token replay, use `scripts/run-qwen-controls.ps1 -Plan
configs/local-qwen-500.example.json -RunDirectory runtime/controls-500`
from the repository root (on one command line). Then run
`scripts/report-qwen-replay.ps1 -RunDirectory runtime/controls-500`.
The report checks each returned model and prompt against the archived baseline,
reads the grade stated in each reply, and preserves the original integer-only score.
