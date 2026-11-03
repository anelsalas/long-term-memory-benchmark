# Summary: Mem0 storage, November 3, 2026

Installed pinned `mem0ai` 3.3.1 and `better-sqlite3` 12.6.2 on native Windows
ARM64 Node 24. An offline test stored two vectors through Mem0's own SQLite-backed
vector store, reopened the database, and retrieved the expected matching record.
The test passed 1/1 with telemetry disabled. No model calls were made. This tests
storage and retrieval; Mem0's extraction and update pipeline still needs testing.
