# Summary: Mem0 comparison condition, November 3, 2026

Connected the C++ `memory_budget` condition to the local memory service. It
records memory IDs, session-level provenance and service configuration separately
from original message IDs. A fake local HTTP integration test failed before the
change and passed afterward. The complete Windows ARM64 offline suite passed
115/115; the preceding Node suite passed 9/9. No live model calls were made.
