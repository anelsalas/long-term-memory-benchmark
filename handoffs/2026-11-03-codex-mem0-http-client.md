# Handoff: memory-service HTTP client, November 3, 2026

Added `memory_service_http.hpp/.cpp` to the Boost retrieval HTTP library and
offline local-server tests. Search returns ranked memory text, date and source
session metadata; the client also reads service configuration. It rejects
non-loopback endpoints. Windows ARM64 CTest passed 114/114. No live calls.

Resume: wire `memory_budget` and service configuration into `ltm-compare` with
session-level provenance that cannot be mistaken for turn evidence. Complete
cross-restart usage limits and prompt provenance before the GPT-only pilot.
