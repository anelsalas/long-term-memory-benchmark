# Task board

Updated October 31, 2026. Status: P01-P05 reviewed on main; paired command implemented; integrated suite passes27/27.
Owner names below describe roles. Agents are activated for bounded assignments.

## Completed

- [x] Independent public repository, objectives and three-paper citations.
- [x] Native C++17 ARM64 library, memory interface and lexical retrieval reference.
- [x] Source identity, person/date filtering, correction-record preservation and JSON output.
- [x] CPU time, resident/private-commit samples and process-lifetime peak memory.
- [x] 27 native CTest checks, including the dataset-to-manifest integration.
- [x] Windows ARM64 CI; pinned application dependency integration.
- [x] Research protocol, contamination-control design and research-owner instructions.

- [x] P01: canonical experiment manifests, SHA-256 identity and complete case accounting.
- [x] P02: selected-case LongMemEval adapter with isolated runtime/evaluator data.

## Next deliverable

One published question and one fresh-fact case, each compared under full-history
and retrieved-evidence conditions with the same model. Save answers, source
evidence, official grades, timing, resources, token usage and cost.

## Ordered implementation queue

| ID | Task | Status | Owner role | Depends on | Completion evidence |
| --- | --- | --- | --- | --- | --- |
| P01 | Versioned experiment manifest and per-case result schema | Complete | Runner | Foundation | Round-trip configuration hashes; expected cases and failed/missing outputs accounted for |
| P02 | Pinned LongMemEval-S dataset adapter | Complete | Data/evaluation | Foundation | Read question/history; isolate gold and annotations; preserve source/date mapping |
| P03 | Provider interface and bounded request lifecycle | Complete | Runner | P01 | Stubbed failures/retries/cancellation; request/token/spend caps; usage and request IDs recorded |
| P04 | Official QA and retrieval scorer adapters | Complete | Data/evaluation | P01, P02 | Parity fixtures against upstream rules, including abstention and source-mapping exclusions |
| P05 | Fresh event-ledger generator and independent gold | Complete | Data/evaluation | P01 | Consistent updates/dates/identities; changed-fact pairs; isolated hidden stories |
| P06 | First live two-case comparison | Command verified; G01 pending | Integrator | P02–P05, G01 | Published and fresh case complete in both conditions; auditable result report |
| P07 | Resumable fixed development subset | Waiting | Integrator | P06 | Interrupted run resumes; matched comparison; failures and contamination controls reported |
| P08 | Complete 500-question LongMemEval-S comparison | Waiting | Integrator/research | P07 | Frozen configuration; category results, uncertainty, human audit and reviewed artifacts |
| P09 | Resource scaling and regression workloads | Ready | Runner/research | P01, P02 | Frozen larger histories, repeated queries, ingestion cost, cold/warm median/p95 and resource records |
| P10 | LoCoMo and independent Mem0/Graphiti comparisons | Waiting | Research/adapters | P08 | Documented configurations, attribution and comparable accuracy/resource tradeoffs |

P03-P05 have integrated test evidence and are published for review. P09 scaling
workloads remain ready for assignment. The [protocol](research/PROTOCOL.md) defines
the comparison; [resource definitions](docs/RESOURCES.md) define measurement scope.

## Live-run gate

- [ ] G01: select accessible model endpoints and versions, verify data-use terms,
  configure credentials in protected storage, and set the pilot request/token/spend cap.

G01 gates live inference. The dataset, schema, stubs, scoring and synthetic
development work can progress independently.

## Updating this board

Complete a task when its acceptance evidence is recorded in reports/ and tests
are observed. Update this board and STATUS.md together. Keep configuration,
source/data/model versions, measured outcomes and remaining dependencies explicit.

Accepted evidence: [P01](reports/p01-experiment.md), [P02](reports/p02-dataset.md),
and [coordinator integration](reports/p01-p02-integration.md).

## Dashboard

D01 UI published at codex/dashboard 4f40c15: Control, Setup, Results and Credits.
The user has started Claude on dashboard continuation.
D02 pending: authenticated native service, setup validation and benchmark control endpoints.
Coordinator owns paired command integration, model access and pilot results.
Core P03-P05 published main454b486; command publication follows verified27/27.

## Current pilot acceptance

- [x] Windows HTTPS adapter and runnable ltm-compare command.
- [x] Fresh two-reader plan and published/fresh six-request plan, zero dispatch.
- [x] Independent CLI checks in native CTest27/27.
- [ ] G01: accessible account/key, exact returned version/token bounds and approved cap.
- [ ] P06: actual responses, grades, usage and audited comparison.
- [ ] Complete user-only corpus ranking connection for official retrieval metrics.

## November1 measured local checkpoint

- [x] Execute local Qwen3-0.6B through explicit no-key/zero-cost loopback profile.
- [x] Record full-history82 correct and top1-retrieval23 incorrect against gold82.
- [x] Observe56/56 original dashboard/core tests and actual browser dry-run completion.
- [x] Integrate dashboard explicit local-run mode; browser run completed2realrequests atcost0.
- [ ] Expand frozen local correction cases; separate reference failure from method changes.
- [ ] Choose a larger-context execution profile for published-history comparisons.

The tiny local smoke is one fresh case in two conditions. P06's published plus
fresh workload and P08's full benchmark remain distinct pending experiments.

## Dashboard integration acceptance

57/57 combined native ARM64 tests passed. Browser completed dryrun and localrun;
full-history answer82 correct, retrieved23 incorrect. Preserve the reference result.
Localrun is opt-in via saved run_mode. Physical phone/LAN check is still pending.

## Four-condition next actions — November 1, 2026

- [x] Run20 correction histories in four conditions with local Qwen:80 requests.
- [x] Publish per-answer evidence, accuracy, timing, tokens and runner resources in dashboard Results.
- [ ] Validate known-evidence answers: compare explicit integer-only/abstention prompts and output allowance.
- [ ] Add recall, dates, identity and absent-fact cases to the fixed suite.
- [ ] Integrate semantic retrieval, Mem0 and Graphiti through the same conditions and scorer.
- [ ] Measure model-worker resources and ingestion/storage cost separately.
- [ ] Run published and held-out workloads with declared adaptations.

## Measured Qwen replay

- [x] Validate Qwen with the first complete correction case and all 20 matched histories at the 500-token reply limit.
- [x] Show the actual replies, stated grades, abstentions and original format score in dashboard Results.
- [ ] Freeze the content-scoring rule for the next method comparison and add Mem0 and Graphiti evidence retrieval.

## Retrieval baseline work — November 1, 2026

- [x] Prepare pinned LongMemEval-S and a reproducible, stratified 60-question development selection.
- [x] Start and smoke-test a native Windows ARM64 local embedding service.
- [ ] Integrate and validate BM25, dense, and hybrid retrieval against the published evidence annotations.
- [ ] Run matched development comparisons and record evidence recall, latency, resource use, answer accuracy and limitations.
- [ ] Compare Mem0 and Graphiti using documented, matched settings.

[Preparation report](reports/2026-11-01-retrieval-preparation.md) records versions and measured scope.

## Retrieval baseline review — November 1, 2026

- [x] Independently build and run 86 ARM64 tests on the retrieval branch.
- [x] Recount 270 answer outcomes and fix mixed-format timestamp diagnostics.
- [ ] Recover the judge-audit sample identifiers, labels and selection seed.
- [ ] Validate answer judgments with an independent reviewer and compare memory methods under matched settings.

## Mem0 benchmark — November 3, 2026

- [x] Add an offline-tested actual model-call cap, including retries and concurrent requests.
- [ ] Implement the bounded system-message bridge and concurrency control.
- [ ] Add the pinned Mem0 sidecar and memory-service benchmark condition with offline tests.
- [ ] Measure one case, project resource and call use, then evaluate the approved ten-question phase.
- [ ] Review results before the full 54-question phase.

Use GPT only; configure no Claude model profiles for these experiments.
