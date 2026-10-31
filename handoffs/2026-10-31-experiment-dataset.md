# Experiment and dataset handoff

Read project continuity, TASKS.md and docs/EXPERIMENT.md plus docs/LONGMEMEVAL.md.
P01/P02 are complete. Native ARM64 Release CTest passed 22/22, including source
checks, nine experiment groups, seven adapter groups and their integration bridge.
The shared hash API is ltm::sha256; public libraries are
LongTermMemoryBenchmark::experiment and LongTermMemoryBenchmark::dataset.

Next: P03 provider lifecycle and P04 scoring in parallel, with P05 fact generation
ready. Use protected credentials and a declared budget before G01 live inference.
Keep evaluator fields out of candidate serialization and count all expected cases.
Use bounded subsets initially; full-data streaming/scaling needs its own measurement.
Raw public-case validation bytes stay in ignored runtime storage. Research ownership
instructions are research/RESEARCH-OWNER.md. No task-owned command remains running.
