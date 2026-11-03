# Summary: Mem0 usage accounting, November 3, 2026

Added a loopback bridge metrics endpoint and sidecar ingestion deltas for CLI
attempts and reported input/output tokens. The sidecar flags failed attempts or
missing token data and avoids attributing values across a bridge restart.
Windows ARM64 offline tests passed: 111/111 C++, 9/9 Node. No live model calls.

Deltas require exclusive use of the bridge during ingestion. Usage on failed
attempts can be unreported, and the call cap resets on bridge restart.
