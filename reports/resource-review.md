# Resource measurement review

Terms are explained in the [glossary](../docs/GLOSSARY.md).

October 31, 2026. Static review of the native resource implementation and API
semantics. Runtime build and test outcomes are recorded by the native worker
and coordinator.

## Findings

The inspected conversions and attribution are consistent with their API sources:

- `src/resources.cpp:24-35`: Windows combines process user/kernel FILETIME counts
  and divides 100-nanosecond units by 10,000 for milliseconds. Current working set,
  private commit and process-lifetime peak working set retain separate byte fields.
- `src/resources.cpp:39-53`: Linux uses `RUSAGE_SELF`, converts timeval seconds and
  microseconds to milliseconds, converts `ru_maxrss` KiB to bytes, and samples
  current resident pages using the configured page size. Private commit remains
  unavailable on that path.
- `src/resources.cpp:57-66`: valid monotonic CPU totals yield an interval delta;
  missing, decreasing or nonfinite samples produce an unavailable delta.
- `src/report.cpp:90-105`: scope is explicitly `local_runner_process`. Child
  processes, remote workers, GPU memory and energy remain separate unavailable
  fields. JSON null denotes unavailable measurements rather than measured zero.

No blocking conversion or process-attribution defect was identified in this pass.

## Interpretation for comparisons

CPU time sums execution across the process's threads. It is distinct from wall
time and can exceed elapsed time during parallel work. The present CPU field
combines user and kernel time; separate mode-specific values would require
additional fields when that distinction becomes an experiment requirement.

Memory before/after values are samples. A temporary allocation between samples
may be missed. The lifetime resident peak includes earlier process activity and
must retain its lifetime label. Private committed bytes describe commit charge,
whereas working set/RSS describes resident pages; compare each quantity by name.
Windows working-set and Linux RSS semantics and accounting differ, so record the
OS and collector when presenting cross-platform numbers.

At `src/main.cpp:18-37`, resource snapshots surround work whose wall timer excludes
some snapshot overhead. This makes coarse CPU deltas unsuitable as precise
per-query CPU estimates for the tiny demonstration. Zero CPU deltas are valid
at timer resolution. Use repeated/batched workloads and record collection
overhead for meaningful cost comparisons. `run_work` contains ingestion and
retrieval; these nested measurements describe overlapping intervals.

The report measures the runner process. External database processes, child model
workers and remote services require their own collectors and explicit scopes.
An API response time can be observed locally without observing remote CPU/RAM.
This boundary should accompany public results and total-resource claims.

## Primary API references

- [GetProcessTimes](https://learn.microsoft.com/en-us/windows/win32/api/processthreadsapi/nf-processthreadsapi-getprocesstimes)
- [PROCESS_MEMORY_COUNTERS_EX](https://learn.microsoft.com/en-us/windows/win32/api/psapi/ns-psapi-process_memory_counters_ex)
- [Linux getrusage](https://man7.org/linux/man-pages/man2/getrusage.2.html)

Reviewed source paths: `resources.hpp`, `resources.cpp`, `report.cpp`, `main.cpp`
and the resource component tests. Findings apply to this implementation snapshot;
the coordinator verifies final build, parsed JSON and actual reported samples.
