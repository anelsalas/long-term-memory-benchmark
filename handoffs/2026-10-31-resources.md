# Resource instrumentation handoff

October 31, 2026. Read project continuity and docs/RESOURCES.md.
New resources.hpp/resources.cpp collect process CPU, resident/private commit and
lifetime peak memory. Resource intervals and JSON distinguish null unavailable
fields, snapshots and CPU deltas. Coordinator observed ARM64 Release 19/19 and
parsed JSON; agent reports document TDD and methodological review.

Next: dataset/provider/scorer ports and frozen workload experiments. CPU for tiny
operations is coarse; batch queries for efficiency comparisons. Instrument child
and remote workers separately. No task-owned background process remains.
