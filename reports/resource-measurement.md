# Resource measurement report

Terms are explained in the [glossary](../docs/GLOSSARY.md).

October 31, 2026. Native process instrumentation and observed component checks.

Added optional process resource samples and stage intervals to the offline JSON
report. C++ callers retain `offline_report(checks)` compatibility and can supply
`offline_report(checks, measurements)`. Header: `resources.hpp`.

## Measurements

- `cpu_total_ms`: cumulative user plus kernel CPU for the local process.
- `cpu_delta_ms`: after minus before, when both samples are finite, nonnegative
  and monotonic. Unsupported, failed or invalid samples yield null.
- `resident_bytes`: sampled working set on Windows, approximate resident pages
  times system page size from Linux `/proc/self/statm`.
- `private_committed_bytes`: Windows process-private committed memory.
- `process_lifetime_peak_resident_bytes`: operating-system high-water mark over
  the process lifetime, separately retained in each sample.
- `wall_ms`: steady-clock elapsed time for the declared stage.

Windows uses `GetProcessTimes` (100 ns units converted to milliseconds) and
`GetProcessMemoryInfo`. Linux uses `getrusage(RUSAGE_SELF)` CPU time and lifetime
maximum RSS (KiB converted to bytes); current resident sampling uses procfs.
Memory samples are collected independently and can change between API reads.
Unsupported platform/metric fields remain null. The process scope includes its
threads and excludes attribution to individual allocations or memory methods.

CLI stages are ingestion, retrieval and run_work. Stage wall boundaries exclude
their before/after resource API calls. CPU sample boundaries include small
instrumentation overhead. run_work includes fixture/search instrumentation and
ends before JSON serialization; overlapping intervals are not summed. Child and
remote workers, GPU memory, energy and persisted-artifact bytes remain explicit
null fields for this in-memory local example.

## Observed checks

- TDD red: resource stubs compiled; three of four new resource checks failed.
- Green: native Windows ARM64 Release CTest passed 19/19 (18 component/unit and
  one CLI integration), including CPU deltas, unavailable samples, JSON nulls,
  invalid timing rejection and real process-counter monotonicity.
- Independent Python parsing verified all resource fields and stage identities.
- One three-record sample: run_work 0.0347 ms; ingestion 0.0126 ms; retrieval
  0.0205 ms. Run-end resident memory was 6,180,864 bytes, private committed memory
  929,792 bytes and process-lifetime resident peak 6,180,864 bytes.
- Sample CPU delta was zero at the OS counter's resolution. These short component
  observations establish instrumentation; larger workloads establish capacity.

Linux source is implemented; Linux execution is delegated to the existing CI
matrix. Native verification used the installed MSVC ARM64/CMake toolchain.

Sources: [GetProcessTimes](https://learn.microsoft.com/en-us/windows/win32/api/processthreadsapi/nf-processthreadsapi-getprocesstimes),
[Windows memory counters](https://learn.microsoft.com/en-us/windows/win32/api/psapi/ns-psapi-process_memory_counters_ex),
[Linux getrusage](https://man7.org/linux/man-pages/man2/getrusage.2.html),
[Linux procfs](https://docs.kernel.org/filesystems/proc.html).

Files changed: CMakeLists.txt, resources.hpp/resources.cpp, report.hpp/report.cpp,
main.cpp, component_tests.cpp, examples/offline-demo.md, and this report.
No model requests, new services or background processes were started.
