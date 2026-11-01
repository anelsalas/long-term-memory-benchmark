# Mem0 first-case pilot

Codex summary, November 1, 2026.

Mem0 OSS 3.3.1 processed the frozen fictional score history, extracted two
memories, and returned both. The first retrieved memory included Lina's
correction from 23 to 82. GPT-6.1-Sol answered 82 using the returned memories.

This is an adapted integration experiment. The Mem0 extraction instruction was
shortened to fit the Dumas model connection, the local embedding is deterministic
128-dimensional token feature hashing, the vector store uses an in-memory
SQLite database, and history and telemetry are disabled. The question requested
up to five memories. The prior keyword comparison used top one. This is one
case, and a fair comparison needs matched retrieval settings, fresh held-out
cases, provenance and resource measurements.

The actual stored and retrieved text is in reports/2026-11-01-mem0-first-case.json
and /mem0-first-case.html on the dashboard. Native ARM64 Node ran the upstream
Mem0 package. Public benchmark source and the private Dumas adapter are separate.
The private adapter and isolated dependency lock are described in the private
handoff. No batch is running.
