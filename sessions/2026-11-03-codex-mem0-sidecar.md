# Summary: Mem0 sidecar contract, November 3, 2026

Added session ingestion, search and local HTTP service functions with tests for
date metadata, repeated sessions, changed-content conflicts, uncertain failure
across restart, and browser/foreign-host refusal. All 6 offline Node tests passed;
they use fake providers and fictional records. No live model calls were made.

The session ledger marks a session pending before Mem0 writes. Interrupted
operations stay uncertain until inspected. Token and call fields remain null;
startup, provenance and benchmark connection remain to be built.
