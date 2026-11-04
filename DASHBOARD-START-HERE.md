# Dashboard continuation guide

Terms are explained in the [glossary](docs/GLOSSARY.md).

October 31, 2026. Codex summary.

Branch: codex/dashboard. Base: core review commit ebc0105.
Core review: REVIEW-START-HERE.md. This branch owns the lightweight responsive
Control, Setup, Results and Credits interface for Surface and cellphone.

## Implementation boundary

Use small same-origin HTML/CSS/JavaScript assets under web/. Native C++17 owns
benchmark execution, authentication, setup validation, model credentials and
resource measurements. docs/DASHBOARD.md defines the UI/API contract once the UI
checkpoint is delivered. reports/dashboard-ui.md records observed UI checks.

## Next acceptance steps

1. Review UI assets and versioned JSON contract; preserve measured/unknown values.
2. Implement authenticated native service and exact control/setup/results routes.
3. Enforce request/spend limits and server-side credential handling.
4. Exercise control/setup/results against real benchmark output.
5. Verify responsive layout and authenticated LAN access on Surface and cellphone.
6. Record measured results and independently review before integration.

Core suite: 25/25 native ARM64 Release CTest passed at branch base. UI/backend
acceptance is recorded separately. Current dashboard branch is an independent
workstream; core source review continues on codex/memory-review.
