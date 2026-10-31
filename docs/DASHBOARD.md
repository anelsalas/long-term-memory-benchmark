# Dashboard interface

Serve `web/` from the authenticated native service with its same-origin `/api`.
The responsive HTML/CSS/JavaScript interface provides Control, Setup, Results and
Credits. Assets use system fonts and local files. Opening the page before the
service exists shows a disconnected state with run actions disabled.

Control submits selected cases and evidence conditions, lists recorded runs and
uses server-provided resume/stop capabilities. Setup edits endpoint/model versions,
request/token/spend caps and seed. Results renders supplied measurements and case
grades; null or missing measurements display Unavailable. Credits links the three
papers, source repositories and licenses, with adaptation guidance.

## API version 1

All JSON responses include `schema_version: 1`. Encode uint64 counters, seeds and
nanodollar amounts as decimal strings. Ratios are finite values from 0 to 1.

| Method and path | Request / response |
| --- | --- |
| GET /api/status | `authenticated`, `csrf_token`, `capabilities: {can_start, can_configure}`, optional `operation_states: {operationId: accepted/rejected/completed}` |
| GET /api/setup | `setup`: saved configuration or null |
| PUT /api/setup | Version, provider, endpoint_url, reader_model_id/version, judge_model_id/version, max_requests, max_tokens, max_cost_nanos, seed; returns `setup` with opaque `revision` |
| GET /api/runs | `runs`: array of id, status, completed, expected, configuration_hash, can_resume, can_stop |
| POST /api/runs | Version, setup_revision, dataset, condition, case_ids; returns `run` with id |
| POST /api/runs/{id}/resume | Version; returns accepted operation JSON |
| POST /api/runs/{id}/stop | Version; returns accepted operation JSON; actual run state comes from GET |
| GET /api/runs/{id}/results | measurement_scope, summary with graded/expected, metrics, cases |

Setup model field names are `reader_model_id`, `reader_model_version`,
`judge_model_id` and `judge_model_version`. Supported UI conditions are full_history,
retrieved_evidence, no_history and direct_evidence; datasets are longmemeval_s and
fresh_facts. The backend validates actual adapters, dataset revisions and cases.

Results metric keys: answer_accuracy (graded subset; show its denominator),
source_recall (declared source metric), retrieval_p50_ms, retrieval_p95_ms,
answer_p50_ms, billable_tokens, cpu_delta_ms, resident_bytes,
lifetime_peak_resident_bytes and cost_nanos. Cases contain case_id, status,
correct (boolean/null) and source_ids. Backend aggregation preserves failed,
missing and ungraded cases. Scope distinguishes measured local, remote and total
resources; absent provider accounting remains null.

## Authenticated operations

Authenticate through the service with a protected same-origin session. The UI
sends same-origin credentials, an ephemeral `X-CSRF-Token` and a unique
`Idempotency-Key` for mutations. The server verifies session/CSRF/origin, authorizes
operations and enforces model access, endpoint allowlists and budgets. Credential
values stay in server-side protected storage. Browser forms accept endpoint URLs
without embedded credentials, query parameters or fragments.

An eight-second HTTP timeout or ambiguous response keeps the operation pending and
disables further mutations. Refresh asks the service to reconcile that operation
through `operation_states`; the client performs no automatic mutation retry.
Backend idempotency and durable operation records remain authoritative across page
reloads. The service must recover unresolved operations before admitting new runs.

Use HTTPS with authentication for phone access; localhost supports local preview.
Run status refreshes every five seconds while an active run is visible. Manual
refresh is available in disconnected and idle states.

## Verification and handoff

Run `node --check web/app.js` and `node web/tests.mjs` for syntax and request/format
checks. The interface currently awaits the native HTTP/auth/operation backend.
Phone browser layout, keyboard flow and end-to-end authenticated run control require
browser verification against that service. Runtime test data comes only from API
responses; the default UI contains no fixture results.
