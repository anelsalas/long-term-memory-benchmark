# Summary: published-case ingestion export, November 3, 2026

Added a C++ exporter that uses the pinned LongMemEval loader, retains opaque IDs,
dates and both message roles, and excludes evaluator annotations. It refuses to
overwrite an output file. A published multi-session case exported 47 sessions and
all 495 messages, including one assistant-leading session. The Windows ARM64
offline suites passed 116/116 C++/integration and 9/9 Node tests. No live calls.
