# Handoff: published-case ingestion export, November 3, 2026

`ltm-memory-export` produces sidecar ingestion JSON from a checksum-verified
LongMemEval question, omitting labels and preserving original message order with
opaque session and round IDs. The sidecar accepts leading assistant messages.
The chosen multi-session case `bb7c3b45` exported all 47 sessions and 495 messages
to an ignored runtime file. No model was called or memory ingested.

Verification: 116/116 Windows ARM64 C++/integration tests and 9/9 Node tests
passed offline. Resume by enforcing the durable cross-restart attempt budget and
recording prompt provenance, then ingest this case with the GPT-only bridge,
measure and project usage, and compare it with the existing baseline.
