# Handoff: Mem0 sidecar contract, November 3, 2026

`adapters/mem0/sidecar.js` implements scoped session ingestion, ranked search,
and an HTTP handler with Host/Origin checks. It serializes ingestion and records
pending or completed sessions in a local SQLite ledger. The code uses per-session
metadata, adds the session date to user input, and requires namespaces without
path separators. Storage calls use the pinned upstream Mem0 implementation.

Verification: 6/6 offline Node tests passed on Windows ARM64, including ledger
restart behavior and local HTTP tests. No live model calls or services remain.

Resume: add loopback-only executable startup, end-to-end HTTP tests using real
Mem0 with fake providers, prompt/version provenance, and actual bridge call/token
accounting; null usage fields are placeholders. Integrate `memory_budget` into
the C++ harness, then run one GPT-only case with the authorized cap.
