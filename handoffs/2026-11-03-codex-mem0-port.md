# Handoff: memory-service port, November 3, 2026

Added `memory_service.hpp` and `memory_service.cpp`, linked into retrieval, with
an offline fake-service behavioral test for budget order, chronological context,
source-session metadata, and missing provenance. Windows ARM64 CTest passed
112/112. The previous Mem0 Node suite passed 9/9; no live services or model calls.

Resume: implement the loopback HTTP adapter and wire `memory_budget` into
`ltm-compare`; record service configuration and session-level evidence separately
from message IDs. Add a durable cross-restart call budget and prompt provenance,
then run one GPT-only published case before the capped ten-question phase.
