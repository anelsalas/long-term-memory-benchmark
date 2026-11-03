# Handoff: Mem0 local HTTP, November 3, 2026

Added `adapters/mem0/server.js` with validated GPT-only profile selection,
loopback endpoints and binding, package-version and installed-bundle hash
provenance. An offline test now sends a dated fictional session through the HTTP
sidecar and actual Mem0 using fake LLM/embedding servers, then retrieves it and
checks replay. All 7 Node tests passed on Windows ARM64. No service remains.

Resume: add reliable CLI-attempt and usage accounting, individual prompt
provenance, and the C++ `memory_budget` condition. Only GPT profiles are allowed
for the authorized measured case. The local HTTP service and bridge remain
reachable by other local processes; use benchmark records and preserve that
limitation in reports.
