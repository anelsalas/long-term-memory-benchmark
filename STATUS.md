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

[Resource definitions](docs/RESOURCES.md) Â· [Schema](docs/EXPERIMENT.md) Â·
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
