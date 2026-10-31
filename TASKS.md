# Task board

Updated October 31, 2026. Status: foundation complete; first model-backed comparison next.
Owner names below describe roles. Agents are activated for bounded assignments.

## Completed

- [x] Independent public repository, objectives and three-paper citations.
- [x] Native C++17 ARM64 library, memory interface and lexical retrieval reference.
- [x] Source identity, person/date filtering, correction-record preservation and JSON output.
- [x] CPU time, resident/private-commit samples and process-lifetime peak memory.
- [x] 19 native checks; the application consumer suite passes 20 including those 19.
- [x] Linux and Windows ARM64 CI; pinned application dependency integration.
- [x] Research protocol, contamination-control design and research-owner instructions.

## Next deliverable

One published question and one fresh-fact case, each compared under full-history
and retrieved-evidence conditions with the same model. Save answers, source
evidence, official grades, timing, resources, token usage and cost.

## Ordered implementation queue

| ID | Task | Owner role | Depends on | Completion evidence |
| --- | --- | --- | --- | --- |
| P01 | Versioned experiment manifest and per-case result schema | Runner | Ready | Round-trip configuration hashes; expected cases and failed/missing outputs accounted for |
| P02 | Pinned LongMemEval-S dataset adapter | Data/evaluation | Ready | Read question/history; isolate gold and annotations; preserve source/date mapping |
| P03 | Provider interface and bounded request lifecycle | Runner | P01 | Stubbed failures/retries/cancellation; request/token/spend caps; usage and request IDs recorded |
| P04 | Official QA and retrieval scorer adapters | Data/evaluation | P01, P02 | Parity fixtures against upstream rules, including abstention and source-mapping exclusions |
| P05 | Fresh event-ledger generator and independent gold | Data/evaluation | P01 | Consistent updates/dates/identities; changed-fact pairs; isolated hidden stories |
| P06 | First live two-case comparison | Integrator | P02–P05, G01 | Published and fresh case complete in both conditions; auditable result report |
| P07 | Resumable fixed development subset | Integrator | P06 | Interrupted run resumes; matched comparison; failures and contamination controls reported |
| P08 | Complete 500-question LongMemEval-S comparison | Integrator/research | P07 | Frozen configuration; category results, uncertainty, human audit and reviewed artifacts |
| P09 | Resource scaling and regression workloads | Runner/research | P01, P02 | Frozen larger histories, repeated queries, ingestion cost, cold/warm median/p95 and resource records |
| P10 | LoCoMo and independent Mem0/Graphiti comparisons | Research/adapters | P08 | Documented configurations, attribution and comparable accuracy/resource tradeoffs |

P01 and P02 can progress in parallel. P09 can progress after its dependencies
while model evaluation is prepared. The [protocol](research/PROTOCOL.md) defines
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
