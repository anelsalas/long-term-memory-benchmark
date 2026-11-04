# P01 experiment schema report

Terms are explained in the [glossary](../docs/GLOSSARY.md).

October 31, 2026. Native ARM64 implementation and observed checks.

Implemented versioned manifests, exact canonical configuration SHA256, typed
lifecycle records and coverage-accounted result serialization. JSON remains an
implementation dependency; public headers expose standard C++17 types.

## Evidence

- Red: compiled native ARM64 stubs; all six initial behavior groups failed.
- Green: all six groups passed after implementation.
- Extended validation: nine groups pass, including 19 independent field mutations,
  invalid schema types/unknown fields/duplicate keys, result-summary mismatches,
  model-version mismatch and pending/ungraded/missing/failed coverage.
- SHA256 known-answer vectors: empty, `abc`, standard multi-block input and one
  million `a` bytes; embedded-null handling is also checked.
- Native Release CTest `ltm.unit.experiments` passed with MSVC 19.44.35229.0 using
  the Visual Studio 2022 ARM64 generator. Test executable runs all groups without
  arguments. Build directory: ignored `build-p01/`.

## Contracts

`sha256(string_view)` returns 64 lowercase hex characters over exact input bytes.
Manifest identity includes every declared experiment field. JSON object ordering
and formatting are canonicalized; ordered case selection and string values remain
significant. Results embed the configuration hash, validate source coverage and
recompute the stored summary on load. See [schema semantics](../docs/EXPERIMENT.md).

The provider lifecycle supplies transitions, calls, timings and usage in P03.
The coordinator owns CMake integration and publication. This task changed only
experiment/hash headers and sources, experiment tests, schema documentation and
this report. Existing application and audio files were preserved. No external
models, installs, services or background processes were started.
