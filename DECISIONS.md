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

## Process resource accounting

Measure wall time, cumulative CPU deltas, resident samples, Windows private commit
and process-lifetime peak resident memory through native OS APIs. Keep snapshots,
intervals and lifetime peaks distinct; include local-runner scope in every report.
Use null for unavailable collectors. Treat the tiny development run as an
instrumentation check; use frozen larger workloads for efficiency comparisons.

## Central task board

Maintain TASKS.md as the ordered implementation queue, with dependencies, owner
roles and acceptance evidence. Keep verified capabilities and next actions in
STATUS.md. Model-access/budget setup gates live inference; offline implementation
work progresses independently.

## Role based research brief

Name the reusable assignment research/RESEARCH-OWNER.md. Instructions describe
the research role and evidence requirements independently of a model vendor.

## Experiment and dataset boundary

Use strict versioned JSON and canonical SHA-256 identity for declared experiment
configuration. Account for expected, missing, failed and ungraded cases explicitly.
Keep candidate history separate from evaluator answers/annotations; replace
original hint-bearing IDs with opaque source IDs and retain evaluator mappings.
Validate exact-byte dataset provenance and bounded selected-case inputs.

## Parser dependency

Vendor JSON for Modern C++ 3.11.3 with its MIT license and official release
checksum. Keep JSON types within implementation files and expose standard C++17
types. Verify the vendored header hash at CMake configuration.

## Surface platform focus

Use native Windows ARM64 with MSVC, CMake and CTest for implementation and CI.
The resource collector uses Windows process APIs. Concentrate validation effort
on the Surface execution target selected by the user.

## Mobile dashboard and deterministic development histories

October 31, 2026. Serve a lightweight responsive dashboard from the Surface,
with Control, Setup, Results and Credits. Use same-origin authenticated API
controls and server-side credential storage. UI work is delegated; backend
integration follows. Generate fresh development histories in C++17 from seeded
structured ledgers with independently resolved expected answers and explicit
correction chains. Combined native ARM64 Release verification passed 25/25.

## Dashboard ownership checkpoint

October 31, 2026. User has started Claude on dashboard continuation. Keep
coordinator work focused on core benchmark review, model access and pilot results.
Dashboard branch and API contract provide the integration boundary.

## Evidence-dependent fictional values

October 31, 2026. Sample other-person values independently and vary correction
values rather than preserving fixed offsets. A failing-to-passing regression
exposed a shortcut that weakened evidence-dependence tests. Keep source revision
with seeds because the draw sequence changed. Coordinator reviewed source paths,
observed3/3 scoped suites and independently matched12 upstream grading prompts.

## Native comparison command and measured boundaries

October31,2026. Expose ltm-compare --config FILE --dry-run/--execute for dashboard
process integration. Dry-run plans declare zero dispatch and null accuracy;
execute uses protected environment credentials, verified token-bound declaration
and global request/token/cost caps. Fresh score grading is deterministic; public
answers use isolated official-compatible judge prompts after the reader artifact
is saved. Record judge-cap changes and lexical retrieval as adaptations.

Public retrieval metrics require a full user-only ranking. Keep their value
unavailable with a reason until that adapter is connected. Record source IDs now.
The new transport and CLI have offline verification27/27; live provider acceptance
and approved pilot spending remain G01. Claude retains dashboard ownership.

## Local zero-cost execution and first measured retrieval failure

November1,2026. User selected the existing local Qwen service and authorized
execution. Enable explicit numeric-loopback local mode with zero prices/cost,
no credential lookup and positive workload caps. Preserve external HTTPS mode.
The live service caps context512bytes/output32tokens; declare compact_local_smoke,
zero distractors, seed42 and top1 selection. First actual responses: full82
correct, retrieved23 incorrect because the old record outranked the correction.
Preserve that result rather than tuning it away. Unmatched cold/warm timings are
reported as observations. Completiontokens are reported; inputtokens unknown and
retained reservations remain separate. Modelworker resources are outside runner
sampling. Claude dashboard source stays preserved; integration uses a new branch.

## Explicit local dashboard execution

November1,2026. Add savedrun_mode dry_run(default)/local. Local requires numeric
loopbackHTTP, zero prices/cost and positive request/token caps; the reviewed
compactplan supplies verifiedlimits. Browser actualrun completed with2 requests
and accuracy0.5. Separate plan-template validation from savedsettings to permit
startup without a configured model. RuntimeRED endpoint_url failure led to a
startup-check regression; finalcombined57/57 passed. Integrate originalClaude
branch and testedcore through a separateworktree, preserving its checkout.

## Four-condition measured control run — November 1, 2026

Use fresh ledger source IDs for known-evidence controls, empty sessions for
question-only controls, and identical prompts across conditions. Rotate order
across20 seeds. Record integer gold agreement, abstention, supporting evidence,
output cap and timing separately. Preserve the all-zero answer result and the
prompt change; validate the reader before using these cases to rank memory systems.

## Qwen reply allowance for correction histories

November 1, 2026. Use 500 as the local output ceiling. Compare the exact earlier
and current prompts, evidence and returned text. Retain the original format score
and label the additional content reading as retrospective. All 80 replies continued
the earlier text and the direct-evidence control stated the correct grade 20/20.

## Sonnet validation

Use the same frozen user messages and the Qwen content-reading rule. Record returned model identity and literal replies. Report evidence supplied and distinguish abstaining with no history from retrieving a superseded grade.

## GPT Sol pilot

Compare the four identical prompt hashes and selected evidence from seed 42. Record the requested catalog model and distinguish that identity from a provider-reported model snapshot.

## Mem0 pilot

Document the compact extraction instruction, deterministic token-feature representation, in-memory storage and top-five retrieval. Compare matched settings before interpreting differences.

## Retrieval development selection and embeddings — November 1, 2026

Pin the cleaned LongMemEval-S revision and select ten cases per question type using a fixed seed and stable hash ranking, reserving an abstention case in eligible types. Use a local Qwen3 embedding model for this adapted baseline and record its revision, normalization, configuration and resource measurements. Compare paper configurations separately.

## Embedding thread setting

Use twelve CPU threads for the current retrieval preparation run; record competing workloads when comparing timings. The original two-thread smoke measurement remains in the preparation report.

## Dataset chronology and repeated IDs

Stable-sort dated sessions and retain repeated source IDs as distinct opaque occurrences. Keep excluding post-question sessions under the existing chronological policy and report excluded cases; compare a full-corpus policy separately. Verify repeated-ID scorer parity against upstream before publication claims.

## Repeated-ID parity check

Keep the authors' evaluation functions external and pin their revision for comparison. The parity fixture covers repeated source IDs, 162 permutations/cutoffs and all three retrieval levels, using standard-library DCG in place of NumPy.

## History time sensitivity

Keep strict history-time validation as the default. Provide an explicit include-all policy for source-corpus fidelity; callers must record it and flag later timestamps. The six affected development cases include five with answer-labelled future sessions, so filtering individual sessions could remove evidence.

## Integer gold in official judge prompts

Render integer JSON gold as decimal text to match upstream Python formatting. Preserve explicit errors for other non-string JSON types until separately validated.

## Retrieval baseline review

Use the dataset adapter's validated timestamp comparison for diagnostics, and treat the reader/retrieval bottleneck as an open hypothesis. A model-reviewed judge sample requires recoverable selection details for independent verification.

## Stronger-reader report review

Present matching source-ID selection, model-judged outcomes and serving differences separately. Report subscription-credit spending as unmeasured and describe the local bridge's access boundary.

## Mem0 bridge call accounting

Count actual CLI process attempts, including retries, against a process-wide atomic `max_calls` cap. A zero or omitted cap is unrestricted; Mem0 experiments must configure a positive value and record restarts.

## Mem0 bridge concurrency

Use `max_concurrent` to queue CLI attempts until a slot is available. Count each
retry separately and release its slot immediately after the process ends. A zero
or omitted limit preserves prior behavior.

## Mem0 request instructions

Keep a leading system message separate from user content. The Codex runner uses
a distinct temporary instructions file per attempt and removes it when the
runner returns. Abrupt process termination can leave a temporary file behind;
experimental input is fictional and cleanup after interruption needs validation.

## Mem0 local storage dependency

Pin `mem0ai` 3.3.1 and `better-sqlite3` 12.6.2. A native Windows ARM64 offline
test verified Mem0's SQLite-backed vector store can persist and retrieve records.
The full Mem0 ingestion and model workflow remains to be tested.

## Mem0 package behavior and provenance

Mem0 OSS 3.3.1's extraction prompt specifies additive memories and its search
combines semantic, keyword and entity-related signals. This version differs from
the paper's original update design and must be reported as a versioned adaptation.
It rejects `timestamp` on OSS add; session dates must be passed as metadata and
in the input. The built-in entity store derives its path by replacing `.db`, so
the adapter requires that suffix to keep its separate database distinct.

## Interrupted Mem0 ingestion

Record each session as pending before calling Mem0, then mark it complete after
the call returns. A pending session after a failure or restart is uncertain and
requires inspection instead of automatic replay, since Mem0's separate writes
and the benchmark ledger cannot be committed in one transaction. Completed
identical sessions can be replayed safely; changed content is rejected.

## Mem0 service startup

Bind the service to IPv4 loopback, reject browser origins and foreign hosts, and
require a GPT profile in the startup configuration. Include the pinned Mem0
version and OSS bundle SHA-256 in the configuration hash. Local processes can
still access the service; use only benchmark records while that boundary is in
place. Per-prompt hashes and model usage accounting remain pending.
