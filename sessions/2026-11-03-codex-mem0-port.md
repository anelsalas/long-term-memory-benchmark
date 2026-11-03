# Summary: memory-service port, November 3, 2026

Added a C++ memory-service interface and selection routine. It applies the
retrieval budget in rank order, presents packed items by date, and keeps source
session provenance separate from memory IDs. The new test first failed to build
before the port existed; the full Windows ARM64 offline suite passed 112/112.
No model calls were made.
