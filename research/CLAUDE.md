# Research owner instructions for Claude or Codex

Own the methodological review for a public benchmark of reliable long-term memory.
Read [PROJECT.md](../PROJECT.md), [STATUS.md](../STATUS.md),
[PROTOCOL.md](PROTOCOL.md) and [SOURCES.md](SOURCES.md), then the latest handoff.
The coordinator assigns work and integrates reviewed evidence.

## First assignment

1. Select one baseline and one retrieval arm for the fixed LongMemEval-S subset.
2. Resolve exact source/data/model versions, prompt construction, scoring and
   context limits. Link every claim to a primary paper section or pinned source.
3. Record each adaptation, its rationale and expected effect. Identify effects
   requiring ablation or measurement before causal attribution.
4. Review the fresh-fact generator's event semantics, gold computation, changed-fact
   consistency, checkpoint timing and hidden-story separation.
5. Specify pilot call/token counts and estimate full-run cost from observed usage.
6. Review the final per-case report for completeness, uncertainty and provenance.

## Deliverables

- `research/reviews/<assignment>-protocol.md`: configuration matrix, scoring
  definition, open questions and concrete acceptance criteria.
- `reports/<assignment>-research.md`: sources inspected, decisions, validations,
  results status and exact next action for the coordinator.

Keep edits within the assigned paths. Propose interface changes through the
coordinator. Use concise, affirmative prose focused on requirements and evidence.
Record research hypotheses, planned comparisons and observed results distinctly.

## Evidence standard

Use matched models, data access and scoring within a comparison. Separate original
paper settings from adapted settings. Test correct source retrieval independently
of answering; assess evidence dependence with no-history, retrieved and direct-source
conditions. Audit a blinded sample, paired uncertainty and category regressions.

Keep hidden evaluation histories, gold and seeds in separately assigned private
storage. Public development cases support implementation and review. Record all
requested cases, failures, retries, cost and cache state. Model calls follow a
declared budget and measured pilot throughput.

## Coordination

This file is a reusable assignment brief. Actual agent activation and completion
are recorded by the coordinator in project status and dated reports.
