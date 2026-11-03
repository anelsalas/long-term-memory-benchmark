# Handoff: durable call budget, November 3, 2026

`budget_ledger_path` plus positive `max_calls` enables file-locked, flushed
attempt reservations before each CLI invocation. The ledger preserves its cap
across restarts, rejects a changed cap and fails closed on corruption. Preserve
the same ledger for the experiment; replacing it starts a new budget. Metrics
remain per-process, and unsuccessful attempts may have unreported tokens.

Windows ARM64 CTest passed 118/118; the prior Node suite passed 9/9. No services
or model calls ran. Resume with prompt provenance and the first GPT-only Mem0
case, using the prepared 47-session export and a positive durable call cap.
