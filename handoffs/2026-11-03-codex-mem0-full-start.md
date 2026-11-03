# Handoff: full Mem0 case started, November 3, 2026

The first session of published case `bb7c3b45` completed with 22 memories, one
GPT call and 116 seconds client wall time. Full 47-session ingestion has started;
its first session was replayed from the ledger. The durable attempt cap is 150.
No Claude profile is configured. Accuracy is pending.

The ignored `runtime/mem0-pilot/` directory contains `runner.json`,
`processes.json`, `full-progress.jsonl`, logs, the sidecar store and the bridge
ledger. Check the actual runner process and progress before taking action. On
failure inspect the pending session and logs before resuming; use a new progress
file and the same ledger. On completion run the `memory_budget` comparison with
the existing GPT reader and judge, record the case's accuracy, latency and usage,
then project against the ten-question cap before proceeding.
