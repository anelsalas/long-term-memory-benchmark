# Summary: Mem0 local HTTP, November 3, 2026

Added GPT-only startup configuration, loopback binding, installed Mem0 bundle
hash provenance, and an end-to-end local HTTP test. The test exercised the real
Mem0 package, SQLite storage, fake model endpoints, ingestion replay and search.
All 7 offline Node tests passed on Windows ARM64. No live model calls were made.

The C++ benchmark integration, individual prompt hashes, and call/token usage
accounting remain. Usage fields return null until accounting is connected.
