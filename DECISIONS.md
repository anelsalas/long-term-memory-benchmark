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
