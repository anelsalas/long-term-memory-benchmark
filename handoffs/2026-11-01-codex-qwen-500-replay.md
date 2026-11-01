# Qwen reply-limit replay

Codex session summary, November 1, 2026.

The 500-token local model service was verified with the normal HTTP endpoint.
The real replay completed 20 histories in four conditions. All 80 current
replies continued the earlier reply. Stated grade agreement was 20/20 for
full history and direct evidence, 0/20 for top-one keywords. All 20 no-history
replies abstained. Strict integer-only agreement stayed 0/20 in every condition.

Files changed include the 500-token config, replay report generator,
machine-readable and narrative reports, Results navigation and dashboard page.
The same native benchmark binary and saved prompts were used. Source and
deployment tests for the revised local service passed on Windows ARM64.
Node web tests passed. The completed page and all four rows were verified in
the browser. Only the resident service and dashboard continue running.

Resume with reports/2026-11-01-qwen-500-replay.md. Use
configs/local-qwen-500.example.json for local runs. Keep the retrospective grade
reading distinct from the original strict scorer. For the next memory method,
declare the answer rule before running and compare retrieved correction IDs on
the same cases. See TASKS.md for broader cases and the published benchmark.
