# Handoff: Mem0 fake-provider integration, November 3, 2026

`adapters/mem0/memory.js` configures the real Mem0 OSS package with telemetry off,
SQLite, separate entity storage and loopback-only model endpoints. A fictional
session was extracted and retrieved through fake local HTTP providers. All 3
Node tests passed on Windows ARM64. No benchmark service or live model was run.

Mem0 3.3.1's additive-only extraction and hybrid/entity search are different
from the paper's original update architecture. OSS rejects `timestamp`; use
session metadata and dated inputs, and disclose that adaptation. `.db` filenames
are required because the upstream entity-store path replacement assumes that
suffix. The full ingestion/resume HTTP adapter and `memory_budget` condition
are pending; run one GPT-only case before the capped ten-question phase.
