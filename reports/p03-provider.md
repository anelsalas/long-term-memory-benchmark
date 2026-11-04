# P03 provider lifecycle report

Terms are explained in the [glossary](../docs/GLOSSARY.md).

October 31, 2026. Native ARM64 implementation and injected-provider evidence.

Implemented standard C++17 provider/request/reply types, synchronous bounded
lifecycle, shared-runner budget locking, per-attempt provenance, integer cost/token
reservations, usage settlement and explicit uncertain/fatal outcomes. Results tie
to P01 manifest identity and a separate complete lifecycle-policy hash.

## Observed checks

- Red: compiled provider stubs; all eight initial behavior groups failed.
- Green: all eight groups passed after implementation.
- Extended checks: twelve groups passed in native Windows ARM64 Release.
- CTest `ltm.unit.provider` passed through the MSVC 19.44 ARM64 toolchain.
- Covered worst-case caps, zero/exhausted budgets, reported success/failure usage,
  unknown usage, bounded safe retries, ambiguous exceptions, fatal error categories,
  cancellation/deadlines, malformed results, integer overflow, policy identity,
  retained request details and concurrent callers under one request cap.

The fake provider supplies deterministic outputs; model grades remain unset.
Live transport, actual provider token/pricing validation and G01 are separate
acceptance gates. [Provider semantics](../docs/PROVIDER.md) define cooperative
cancellation, post-deadline results, accounting uncertainty and retry contracts.

Files: provider.hpp/provider.cpp, provider_tests.cpp, docs/PROVIDER.md and this
report. Coordinator owns CMake/root integration and publication. Build directory:
ignored build-p03/. No credential reads, model calls, purchases or background
services were performed by this task.

## Readability review

Expanded the header, implementation and tests to consistent C++17 formatting with
one statement per line, explicit guard braces and named policy fixture fields.
Replaced abbreviated arithmetic/encoding helpers with meaningful names and
extracted focused budget-reservation, usage-settlement and reply-normalization
operations. Public types, lifecycle identity and observable outcomes are preserved.

Reviewed overflow, malformed UTF-8, cancellation/deadline and reservation paths;
the existing twelve behavior groups remain the regression checks. A temporary
refactor spelling error was caught by the compiler and corrected before final
validation. The rebuilt native ARM64 Release executable passed all twelve groups;
scoped CTest `ltm.unit.provider` passed after the final source changes. No additional
behavior or tests were introduced by this readability pass.
