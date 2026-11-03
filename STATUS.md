## Mem0 context-budget correction — November 3, 2026

Mem0 context now renders chronological dated text and counts the complete rendered block against the configured estimate. The Windows ARM64 suite passed 118/118, followed by two focused tests including the actual comparison prompt. The corrected first case was judged correct using two GPT calls; its reader used 10,978 input tokens versus dense's 10,894. The approved ten-case comparison has started with the first remaining 53-session history. A durable 1,449-call allowance plus the 51 calls already used enforces the 1,500-call total.

## Mem0 benchmark implementation — November 3, 2026

The first GPT-only Mem0 pilot completed on one published multi-session question. GPT judged both Mem0 and the existing dense baseline correct. Mem0 ingested 47 sessions using 47 calls and 741,357 reported input tokens; its reader used 17,363 input tokens versus 10,894 for the dense baseline. The [pilot report](reports/2026-11-03-mem0-pilot.md) records timing, resources and limitations. Windows ARM64 offline suites passed 118/118 C++/integration and 13/13 Node tests. The ten-question comparison is next.

## Stronger-reader report review — November 2, 2026

Independently verified 105/105 ARM64 tests, 270 matched source-ID pairs, 15 answer-summary totals and 803 cross-judge records on frozen 95e6534. The public report identifies serving-stack differences, unmeasured credit spending and judge limitations. The loopback bridge is stopped; its current access controls allow local processes to submit requests.

## Retrieval baseline integration — November 1, 2026

The frozen retrieval branch was independently built and passed 86/86 ARM64 tests. Five answer-summary counts were verified against 270 recorded outcomes; the local judge remains provisional. An ISO timestamp diagnostic was corrected with behavioral tests. The judge-audit sample details are pending. See [the benchmark report](reports/2026-11-01-retrieval-baselines.md).

## Mem0 first case — November 1, 2026

Mem0 stored and retrieved the fictional correction, and Sol answered 82. The adapted setup and synthetic evidence are in reports/2026-11-01-mem0-first-case.json and dashboard /mem0-first-case.html.

## GPT Sol validation — November 1, 2026

One frozen history, four GPT-6.1-Sol requests. The actual replies were UNKNOWN, 82, 23 and 82 for question only, full history, keyword retrieval and correction alone. All four matched Sonnet. The selected Sol model comes from the account catalog. The relay does not report a distinct serving snapshot.

## Sonnet validation — November 1, 2026

Sonnet 5.5 completed all four conditions for the first frozen history. It answered UNKNOWN, 82, 23 and 82 with no history, full history, keyword retrieval and direct evidence respectively. Read the actual replies in reports/2026-11-01-sonnet-first-case.json and dashboard /sonnet-first-case.html.

## Qwen 500-token replay — November 1, 2026

All 80 matched requests completed. Qwen stated the corrected grade in 20/20
full-history and 20/20 direct-evidence answers. Keyword retrieval missed all
20 corrections. Question-only answers abstained in all 20 cases. The original
strict integer score and the retrospective content reading are both visible in
[the report](reports/2026-11-01-qwen-500-replay.md) and dashboard Results.

## Qwen four-condition results — November 1, 2026

Completed 80 local requests across 20 correction histories. All responses reached
32 tokens; exact gold agreement was 0/20 in each condition. Full and known evidence
contained support20/20; keyword top1 support0/20. See reports/2026-11-01-qwen-controls.md
and dashboard /comparison.html. Native tests57/57 and Node checks passed.
Next: validate known-evidence answering with a declared prompt/output profile,
then compare semantic retrieval, Mem0 and Graphiti on frozen cases.
Dashboard LAN access is user-confirmed; the existing service remains running.
# Current progress

Updated November 1, 2026. [Task board](TASKS.md).

## Latest measured run

Local Qwen3-0.6B completed two requests through the native command with APIcost0.
Full history answered82 correctly. Lexical top1 retrieved the original record,
answered23 and missed the correction. One fresh question/two conditions:1/2correct.
[Measured report](reports/2026-11-01-local-qwen-smoke.md) records scope and timings.

Coordinator reran Claude's original checkout:56/56 native tests passed. The
integrated dashboard/local runner passed57/57 after a startup regression fix.
Browser SaveSetup/Startdryrun/Output passed, then explicit Startlocalrun completed
with2 real Qwen requests, accuracy0.5 and APIcost0. Both observed runs are saved.
Dashboard is active on http://127.0.0.1:18080/ (PID14676), loopback-only.
[Browser acceptance](reports/2026-11-01-browser-acceptance.md).

## Verified

Target platform: native Windows ARM64 with MSVC and CMake/CTest.

- Native C++17 ARM64 core, lexical reference, source/date/person filters and CPU/RAM reports.
- P01: versioned manifests, canonical configuration SHA-256, case lifecycle records and complete coverage summaries.
- P02: provenance-checked LongMemEval selected-case loading, opaque runtime IDs and separate evaluator gold/mappings.
- Independent Release ARM64 verification: 27/27 CTest checks passed.
- Extracted public-schema case validated: 53 sessions and 550 messages; dialogue stays in ignored runtime storage.
- Research protocol, paper citations, integration guide and model-independent [research brief](research/RESEARCH-OWNER.md).

P03 bounded provider lifecycle and P04 official-compatible scoring are implemented.
P05 seeded fictional event ledgers compute expected answers and coherent corrections.
The combined native Release suite passed 27/27 checks after command integration.
Agent reports record observed behavioral TDD and scoped parity checks.

## Next

P03-P05 review is accepted with a fixed generator shortcut regression.
Coordinator observed 3/3 reviewer suites passing and independently checked
12 grader prompts against the pinned upstream function. The user has started Claude on
the separate codex/dashboard workstream. Coordinator focus is core review,
model-selection/access and the first measured comparison. G01 selects
model access and a pilot budget before live inference. P06 then compares one
published question and one fresh case under fixed conditions.

[Resource definitions](docs/RESOURCES.md) · [Schema](docs/EXPERIMENT.md) ·
[Dataset adapter](docs/LONGMEMEVAL.md)

`ltm-compare` and the Windows HTTPS transport compile. Fresh and selected-public
case dry runs produce2/6 planned requests with zero dispatch and isolated artifacts.
Independent CLI acceptance covers candidate fields, null accuracy, budget refusal,
malformed setup and unique output directories. CTest27/27 passed.

The current comparison is an adapted lexical reference. Public-case official
retrieval metrics remain unavailable until its complete-user-ranking adapter is
connected; evidence IDs remain recorded. Live API execution and returned model/
usage acceptance follow G01. The configured transport has offline injected tests;
paid calls remain zero. Local loopback Qwen execution is now observed; cloud access is optional.

## Retrieval baseline preparation — November 1, 2026

Pinned LongMemEval-S data and a 60-question stratified development selection are ready. A Windows ARM64 CPU embedding endpoint produced four normalized 1,024-dimensional vectors; the warm short-text sample ran at 2.34 texts/s. Retrieval comparison and answer evaluation are next. Details and selection: [preparation report](reports/2026-11-01-retrieval-preparation.md).

## Embedding throughput check — November 1, 2026

A 32-short-text batch returned all vectors in 7.874 s with two CPU threads and 2.106 s with twelve in separate runs. Long input throughput and concurrent workload effects require measurement.

## Dataset loader correction — November 1, 2026

The loader sorts sessions chronologically and retains repeated original session IDs as distinct opaque occurrences. The scorer accepts those mappings. New behavioral tests and validation of three affected published cases passed. Sessions dated after a question remain excluded pending a declared policy; official metric parity for repeated IDs needs further verification.

## Repeated-ID scorer parity — November 1, 2026

A synthetic repeated-ID fixture matched the pinned upstream retrieval evaluation across 162 rankings and cutoffs at session, turn, and turn-to-session levels. The upstream NumPy DCG calculation was replaced by an equivalent standard-library function for this check. Loader and scorer CTest groups passed. Post-question-date cases still require a declared policy.

## Explicit history-time policy — November 1, 2026

The loader offers a documented opt-in to preserve all source sessions while retaining the strict default. All six development cases with later timestamps loaded using that opt-in; the existing ARM64 suite passed 53/53. Their answer/retrieval metrics still require a separately labelled run.

## Integer benchmark answers — November 1, 2026

The official judge-prompt adapter supports integer gold values as decimal text. Behavioral tests first reproduced the prior failure and then passed; the configured ARM64 suite passed 53/53. The pinned 500-question dataset has 32 integer-valued answers. Claude can integrate the fix and retry affected cases.
