# Handoff: per-request instructions, November 3, 2026

Changed the bridge, Windows runner, unit and integration tests, and documentation
to carry an optional leading system message. Codex gets a per-attempt temporary
instructions file; the runner deletes it on normal return and C++ unwinding.
Abrupt termination can leave a file in the temporary directory. Existing
user-only requests continue using the configured default.

Verification: 110/110 Windows ARM64 offline tests passed with local socket and
temporary-file access. No live model or benchmark services were started.

Resume: validate the Mem0 3.3.1 native SQLite dependency on Windows ARM64,
implement the pinned sidecar and memory-service benchmark condition, then run
the authorized one-question GPT-only phase. Measure ingestion before the
ten-question phase; obtain user review before extending to 54 questions.
