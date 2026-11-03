# Mem0 adapter dependencies

This adapter uses the open-source package [Mem0](https://github.com/mem0ai/mem0)
(`mem0ai` 3.3.1) and `better-sqlite3` 12.6.2, pinned in the lockfile.
Run `npm ci` to install and `npm test` to check local SQLite vector persistence.
The offline tests use fictional data and local fake model endpoints. The adapter
turns Mem0 telemetry off before loading the package, requires local provider
endpoints, and uses `.db` filenames so Mem0's separate entity store gets its own
database. Mem0 3.3.1 uses additive extraction and hybrid search; report its
version when comparing it with earlier paper results. The benchmark HTTP
integration is under development.

The sidecar service exposes ingestion and ranked search through a local HTTP
contract. A SQLite ledger records each session before ingestion and rejects
automatic replay when an interrupted session may have partially written. The
current usage fields are `null` until model-call accounting is connected.

Start with `node server.js CONFIG_FILE RUNTIME_DIR PORT`, where the JSON config
contains `model`, `llmBaseURL`, `embeddingBaseURL`, `embeddingModel`, and
`dimensions`. Startup requires a GPT profile and loopback endpoints. The server
binds to `127.0.0.1`; local processes can access it. `/config` records the pinned
Mem0 version and the SHA-256 of its installed OSS bundle. The benchmark
connection and per-prompt hashes are under development.
