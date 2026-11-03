# Summary: Mem0 with fake providers, November 3, 2026

Implemented the pinned Mem0 configuration for local model endpoints and tested
real upstream extraction, SQLite storage, metadata, and search using local fake
LLM and embedding responses. All 3 offline Node tests passed. No live model calls.

Mem0 3.3.1 uses additive-only extraction and semantic, keyword, and entity search;
this is a versioned adaptation relative to the paper's earlier update design.
Its OSS `timestamp` option is rejected, and a `.db` suffix keeps its entity store
separate from its memory store. The sidecar and benchmark port are next.
