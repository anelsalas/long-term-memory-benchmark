# Handoff: Mem0 usage accounting, November 3, 2026

The bridge exposes per-instance aggregate attempts, completions, reported tokens,
and missing-usage counts on loopback `/metrics`. The sidecar snapshots those
counters around ingestion and reports deltas; it flags missing usage, failed
attempts, or a restart. Keep the bridge exclusive to ingestion while measuring.

Verification: the Windows ARM64 offline suite passed 111/111 C++ and 9/9 Node
tests. No model calls or benchmark services were started. C++ `memory_budget`
integration, prompt provenance, a persistent cross-restart budget, and the first
live GPT-only case remain. Run one case before considering the ten-question phase.
