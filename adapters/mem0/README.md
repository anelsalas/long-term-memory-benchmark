# Mem0 adapter dependencies

This adapter uses the open-source package [Mem0](https://github.com/mem0ai/mem0)
(`mem0ai` 3.3.1) and `better-sqlite3` 12.6.2, pinned in the lockfile.
Run `npm ci` to install and `npm test` to check local SQLite vector persistence.
The offline tests use fictional data and local fake model endpoints. The adapter
turns Mem0 telemetry off before loading the package, requires local provider
endpoints, and uses `.db` filenames so Mem0's separate entity store gets its own
database. Mem0 3.3.1 uses additive extraction and hybrid search; report its
version when comparing it with earlier paper results.

The sidecar service exposes ingestion and ranked search through a local HTTP
contract. A SQLite ledger records each session before ingestion and rejects
automatic replay when an interrupted session may have partially written. The
service snapshots bridge counters to report call and token deltas. Use the
bridge exclusively during ingestion so deltas are attributable to Mem0. Failed
attempts can have unreported token use; the response marks usage incomplete.
If counters become unavailable or the bridge restarts, usage is unknown. Per-session
usage and system-prompt fingerprints persist in the ledger for safe replay.

Start with `node server.js CONFIG_FILE RUNTIME_DIR PORT`, where the JSON config
contains `model`, `llmBaseURL`, `embeddingBaseURL`, `embeddingModel`, and
`dimensions`. Startup requires a GPT profile and loopback endpoints. The server
binds to `127.0.0.1`; local processes can access it. `/config` records the pinned
Mem0 version and the SHA-256 of its installed OSS bundle.

`ltm-memory-export` converts a checksum-verified published question to the
sidecar's ingestion JSON, preserving opaque session IDs and all user and assistant
messages. It writes a new output file and omits evaluator annotations. Its inputs
are `--dataset`, `--sha256`, `--question`, `--namespace`, and `--out`.

`node ingest.js INPUT_JSON SERVICE_URL OUTPUT_JSONL` sends one session at a time
and saves dialogue-free progress. Use a new output file when resuming; completed
sessions are replayed from the ledger without new model calls.

`node report.js CASE_ID SESSION_COUNT COMPARISON_DIR BASELINE_JSON OUTPUT_JSON
PROGRESS_JSONL...` creates a dialogue-free single-case comparison after ingestion
and judging finish. It counts replayed sessions once and marks incomplete usage.
