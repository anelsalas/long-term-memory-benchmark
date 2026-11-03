# Handoff: Mem0 comparison condition, November 3, 2026

`ltm-compare` now supports `memory_budget` with a local memory-service URL,
namespace and context budget for one published case. It records service config,
memory IDs and session-level evidence separately from original source IDs. An
offline fake-HTTP integration test and the full Windows ARM64 suite passed
115/115. The previous Node suite passed 9/9. No live services or model calls ran.

Resume: prepare and ingest the published case into the sidecar using the loader's
opaque session IDs, add persistent cross-restart attempt accounting and prompt
provenance, and run the authorized one-case GPT-only measurement. Preserve the
representation difference between extracted memory and conversation turns.
