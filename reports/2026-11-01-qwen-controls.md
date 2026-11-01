# Qwen four-condition comparison

November 1, 2026. Twenty fresh correction histories, seeds 42–61, four evidence
conditions each: 80 completed local model requests, API cost $0.

| Condition | Exact gold answers | Supporting evidence present | Median reader time |
|---|---:|---:|---:|
| Question only | 0/20 | 0/20 | 2579.1 ms |
| Full history | 0/20 | 20/20 | 5152.8 ms |
| Keyword top-1 | 0/20 | 0/20 | 2960.0 ms |
| Known evidence | 0/20 | 20/20 | 3022.6 ms |

Every response used all 32 allowed output tokens. Responses contain verbose,
frequently unfinished prose rather than the requested integer. Known-evidence
failures expose an answer-generation/format limit; keyword retrieval separately
misses corrections. Inspect raw answers and prompts in the adjacent JSON report.
Question-only gold agreement is a guessing diagnostic; exact UNKNOWN abstentions
were also 0/20. Natural-language abstention requires a separate semantic audit.

This run adds “or UNKNOWN if absent” to the earlier integer-only instruction.
That prompt change is a candidate explanation for the changed behavior, requiring
a controlled follow-up. Preserve this run; first validate answering with known
evidence under an explicit prompt/output profile, then compare memory methods.

Model: qwen3-0.6b-q8_0. Service bounds: 512 context bytes, 32 output tokens.
Rotating order balances four conditions across positions. The first request includes
loading; timings are descriptive. All cases share one correction template with
varied scores. The exported report includes source hash, prompts, selected evidence,
answers, configuration hashes, output counts and runner CPU/RAM samples.
Input tokens are unavailable. Model-worker CPU/RAM and energy need separate measurement.

Validation: the new four-condition CLI regression failed before implementation,
then passed; all 57 native CTests passed. Additional empty-context/duplicate-condition
checks passed. Existing Node web tests passed. This is a compact development
comparison; published and held-out workloads remain on TASKS.md.

Reproduce using scripts/run-qwen-controls.ps1 and scripts/report-qwen-controls.ps1.
Results appear at /comparison.html on the dashboard, with a Results navigation link.
