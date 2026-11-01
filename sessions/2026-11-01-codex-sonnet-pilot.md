# Sonnet first-case benchmark

Codex summary. November 1, 2026.

Sonnet 5.5 completed all four conditions for the first frozen history. It answered UNKNOWN, 82, 23 and 82 with no history, full history, keyword retrieval and direct evidence respectively. Read the actual replies in reports/2026-11-01-sonnet-first-case.json and dashboard /sonnet-first-case.html.

New scripts/thinking-model.mjs accepts an in-memory reader.answer(messages) implementation. Its regression checks match all 80 earlier Qwen content readings and exclude evaluator fields from dispatch. One synthetic history and four fresh Sonnet replies were observed. Model-specific system instructions and output bounds differ. The browser displayed the completed table. No experiment is running. For another reader, reuse the same validation function and freeze prompt hashes before dispatch. Mem0 and Graphiti comparisons remain pending.
