# Codex retrieval preparation — November 1, 2026

Summary: pinned LongMemEval-S and prepared a six-type 60-case development selection. Built and smoke-tested a local ARM64 Qwen embedding server. See [the report](../reports/2026-11-01-retrieval-preparation.md).

Next: integrate the retrieval branch, verify scorer and provider behavior, then run the matched development comparison. The local server is on loopback port 8771; its runtime data and model are outside the repository. Claude owns retrieval selection and condition parsing during integration.
