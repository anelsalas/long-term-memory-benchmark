# Handoff: Mem0 ingestion runner, November 3, 2026

`adapters/mem0/ingest.js` sends one session per request and writes a new,
flushed, dialogue-free JSONL progress file. The sidecar persists per-session
usage and observed system-prompt fingerprints, including across safe replays.
Bridge counters are attributable only while ingestion has exclusive bridge use;
failed attempts may have unreported tokens.

Windows ARM64 offline suites passed 118/118 C++/integration and 11/11 Node
tests. No live model calls or services ran. Resume by preparing GPT-only bridge
and local embedding configurations with the durable cap, run the prepared
one-case pilot, inspect answer and usage, then project the ten-question cost.
