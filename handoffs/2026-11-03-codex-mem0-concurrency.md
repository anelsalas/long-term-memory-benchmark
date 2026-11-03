# Handoff: Mem0 bridge concurrency, November 3, 2026

The bridge now accepts `max_concurrent` (zero or absent means unrestricted).
Each CLI attempt waits for a slot; retries take separate slots and count against
`max_calls`. Changes are in the bridge header and implementation, tests, CMake,
and bridge documentation.

Verification: the concurrency test failed before implementation; the complete
Windows ARM64 offline suite passed 108/108 afterward with local socket and temp
file permissions. The sandboxed full-suite attempt had filesystem and socket
permission failures; the normal Windows run passed. No benchmark services or
model calls were started.

Resume: implement isolated per-request system messages, then the pinned Mem0
adapter and `memory_budget` condition. Use only GPT profiles for live Mem0
experiments. Measure one question before the ten-question phase, observing the
1,500-attempt limit; obtain user review before extending to 54 questions.
