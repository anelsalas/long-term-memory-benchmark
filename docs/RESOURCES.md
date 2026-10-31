# Resource measurements

Compare methods using the same workload, build, hardware, concurrency and cache
state. Capture ingestion and retrieval separately, alongside answer quality.

| Measurement | Meaning |
| --- | --- |
| Wall time, milliseconds | Elapsed stage duration measured with a steady clock |
| CPU time, milliseconds | Change in process user-plus-kernel CPU time across the interval |
| Resident memory, bytes | Sampled process working set/RSS before and after the stage |
| Private committed memory, bytes | Windows process private commit at each sample |
| Lifetime peak resident memory, bytes | Highest resident value reported for this process through the sample |

The measurement scope is the local runner process and its threads. Process memory
includes the harness, runtime, loaded libraries and candidate memory method.
Use separate worker instrumentation when a method runs in another process or on
a remote service. Store unavailable counters as JSON null with their scope.

CPU time sums activity across threads. It can exceed elapsed time when several
cores work concurrently. Short operations can show zero CPU delta at the operating
system's accounting resolution. Repeat declared workloads to characterize their
resource cost and report the repetitions with timing.

Resident values are samples. A lifetime peak is cumulative across earlier stages;
it is distinct from the peak of an individual interval. Use fresh processes for
isolated peak-memory comparisons and record baseline samples. Keep measurement
overhead and concurrent background activity in the experiment record. CPU samples
include sampling overhead around work-only wall timers. The containing `run_work`
interval includes ingestion and retrieval; report these nested stages separately.

## Native sources

- Windows: [GetProcessTimes](https://learn.microsoft.com/en-us/windows/win32/api/processthreadsapi/nf-processthreadsapi-getprocesstimes),
  [GetProcessMemoryInfo](https://learn.microsoft.com/en-us/windows/win32/api/psapi/nf-psapi-getprocessmemoryinfo),
  [PROCESS_MEMORY_COUNTERS_EX](https://learn.microsoft.com/en-us/windows/win32/api/psapi/ns-psapi-process_memory_counters_ex).
- Linux: `getrusage(RUSAGE_SELF)` for CPU and lifetime peak RSS; sampled RSS from
  `/proc/self/statm`. Values are normalized to bytes/milliseconds.
  [Kernel process accounting documentation](https://docs.kernel.org/filesystems/proc.html).

Persisted index size, model-worker GPU usage, network traffic and energy require
their own collectors. Record those fields when the corresponding adapter can
observe them. Report API token usage and billing separately from local resources.

The three-record offline fixture validates collection and reporting. Resource
scaling studies use larger frozen histories and repeated declared queries.
