# Research protocol review

October 31, 2026. Research documentation assignment completed.

## Delivered

- `research/PROTOCOL.md`: ranked experiments, matched comparisons, metrics,
  synthetic-fact controls, uncertainty and publication procedure.
- `research/SOURCES.md`: three foundation-paper citations, pinned source
  references and explicit historical/adapted reproduction choices.
- `research/CLAUDE.md`: bounded research-owner brief usable by Claude or Codex.

## Evidence inspected

Read project instructions/objectives/status and the recorded upstream source
audit. Verified each foundation paper's authors, title, arXiv version and date
against its primary arXiv record. LongMemEval's record identifies ICLR 2025.
Reviewed the recorded public code references for scorer, runner and graph-eval
roles and selected the minimum first comparison from that evidence.

## Decisions

Prioritize LongMemEval-S end-to-end comparison and fresh-fact controls; use LoCoMo
as the second workload and M retrieval as a separate diagnostic. Preserve
official scoring and declare current defaults explicitly. Derive factual gold
from event records and reserve hidden stories outside public source control.

## Remaining reproduction work

Resolve original dataset revisions, exact paper-era implementation/model access,
hosted Zep/Mem0 configuration and formatted LoCoMo conversion. Validate candidate
adapters against official scoring and serialized-request isolation. Model-backed
experiments and numerical improvements remain planned.

## Validation and handoff

Local relative links, paper IDs, revision formats and protocol length are checked
by the assignment's validation command. The coordinator should review these
documents alongside the native foundation, update root progress and integrate
the next fixed-subset task. This assignment performed source/document review;
model access, paid inference and empirical benchmark scoring await the next stage.

## Provisional native source review

Read the worker's initial `memory.hpp`, `report.hpp`, three `src/*.cpp` files,
`tests/component_tests.cpp` and current root README/PROJECT while implementation
was ongoing. This pass is a static methodological review; build execution and
final source verification belong to the coordinator/native worker.

The JSON identifies `offline_component`, `public_synthetic_development` and
`ascii_lexical_overlap`, records zero model calls, and marks model accuracy/tokens
as null (`src/report.cpp:35`). That accurately scopes this executable. Demo inputs
are public fictional records; its query excludes grade values and the report
derives coverage from expected source IDs only after retrieval. This inspected
path exposes no model request or hidden evaluation data.

The lexical implementation preserves source records, filters person/time before
ranking and uses recency only to break equal lexical scores. The tests verify
those behaviors. Their correction-order check establishes source preservation
and tie ordering, rather than general correction reasoning. `source_recall` is
fractional expected-source coverage (`src/report.cpp:45`); `all_sources_recalled`
is a separate Boolean. Official LongMemEval any/all-source aggregates and
turn/session mapping remain future dataset-adapter work.

Suggested documentation refinement: add one current-executable sentence near
README's run commands naming the public synthetic lexical component check and
its report scope. README/PROJECT objectives currently describe planned end-to-end
metrics appropriately, but readers benefit from that immediate execution label.
Keep timestamps/context semantics and lexical rank order documented as the native
API develops. Build correctness, official metric parity, model answer quality and
contamination controls each require their corresponding later validation.

No substantive methodological or leakage defect was identified in this bounded
source pass. Findings apply to the inspected implementation snapshot.
