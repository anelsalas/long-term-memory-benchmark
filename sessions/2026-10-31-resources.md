# Resource instrumentation summary

User requested resource usage. An implementation agent added Windows/Linux process
collectors and stage reporting; a research agent reviewed units and attribution.
Coordinator independently built ARM64 Release, observed 19/19 CTest passing and
parsed real CLI snapshots/deltas. Resources cover the runner process. Peak memory
is process lifetime, and CPU sampling overhead is documented. Data/model benchmark
and broader worker/storage collectors remain the next implementation stages.
