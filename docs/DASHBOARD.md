# Dashboard

Terms are explained in the [glossary](GLOSSARY.md).

A thin local control panel for benchmark runs: save the model setup and limits,
start and stop runs, and read their output. [ltm-compare](COMPARE.md) does the
work; the saved mode selects a dry plan or a real local-model comparison.

## Build and run

The dashboard server needs Boost headers (Beast/Asio). The benchmark core does not.

```powershell
cmake -S . -B build-vs2026 -G "Visual Studio 18 2026" -A ARM64 -DLTM_BOOST_INCLUDE_DIR=<boost-root>
cmake --build build-vs2026 --config Release
.\build-vs2026\Release\long-term-memory-dashboard.exe
```

Open http://127.0.0.1:18080/. Options (`--help` lists them):

| Option | Default |
| --- | --- |
| `--bind ADDRESS` | `127.0.0.1`. Use the computer's LAN address to reach it from a phone on the same network. Anyone on that network can then use it. |
| `--port PORT` | `18080` |
| `--data DIR` | `runtime/dashboard` (git-ignored): `settings.json` and `runs/` |
| `--web DIR` | this repository's `web/` |
| `--plan FILE` | `configs/pilot-fresh-plan.example.json`: the reviewed ltm-compare plan each run starts from |
| `-- COMMAND...` | `ltm-compare` next to the dashboard |

Each run copies the plan, fills in the saved endpoint, reader and judge
(`id`, `version`), request/token/cost limits and seed, sets `output_directory` to
`artifacts`, writes it to the run folder as `config.json` and starts
`COMMAND --config <run folder>/config.json --dry-run` by default. Saved
`run_mode:"local"` selects `--execute`, the numeric loopback endpoint and zero
prices. It requires a reviewed `compact_local_smoke` plan with verified token
bounds. Launch with `--plan configs/local-qwen-smoke.example.json` for that
profile. Local configuration omits the API-key variable and retains reviewed
context/output bounds, rendering, timeouts and retrieval settings.

In Setup, select **Local · run model for real · $0**, enter the local model and
endpoint, set positive request/token limits, then save. Control shows
**Start local run**. Its output contains actual paired results and failures.
The dry-run mode shows **Start dry run** and produces a plan with zero calls.
Ctrl+C stops the dashboard and ends any active run.

## Runs

One run at a time. `runs/run-N/` holds `settings.json` (the setup used),
`config.json` (the ltm-compare configuration), `output.txt` (stdout and stderr),
`artifacts/` (the plan, manifest and evaluator files) and, once the run ends, `status.txt`.
Statuses: running, completed (exit 0), failed (other exit codes; ltm-compare uses 2 for configuration errors), stopped
(Stop pressed) and interrupted (the dashboard closed while the run was active).
Run numbers are never reused. On Windows each run is placed in a kill-on-close
Job Object, so Stop or closing the dashboard ends the benchmark and any processes
it started. The run inherits only its output file.

## Settings

Saved settings are validated in C++ before they are written:

- `run_mode`: `dry_run` (default, including older saved settings) or `local`.
- `endpoint_url`: full https chat-completions URL for dry runs; local execution
  accepts http with numeric `127.0.0.1` or `[::1]` only. Credentials, query and
  fragment are rejected.
- `reader` and `judge`: nonblank `provider`, `id` and `version`. The plan receives
  `id` and `version` (the exact model sent on the wire); `provider` is recorded with the run.
- `max_requests`, `max_tokens`, `max_cost_nanos` (USD × 10^9) and `seed`: uint64
  decimal strings.
- Local mode requires positive request/token limits and zero cost. C++ enforces
  these conditions, clears credential configuration and sets all model prices
  to zero. The browser displays a read-only zero cost field in local mode.

Local calls use the existing loopback service without a key. External endpoints
remain available for dry planning. Each run stores its saved mode with settings.

## HTTP API

JSON unless noted. Errors are `{"error": "..."}`.

| Method and path | Result |
| --- | --- |
| GET /api/settings | `{"settings": {...} or null}` |
| PUT /api/settings | Saves and returns `{"settings": {...}}`; 400 with the reason if invalid |
| GET /api/runs | `{"runs": [{"id", "status", "exit_code"}]}` |
| POST /api/runs | 201 `{"run": {...}}`; 409 without saved settings or while a run is active |
| POST /api/runs/{id}/stop | 200; stopping a finished run changes nothing; 404 for unknown runs |
| GET /api/runs/{id}/output | `text/plain`, the last 256 KiB |
| GET /, /index.html, /app.js, /styles.css, /icon.svg | Page files |

There is no login. Two cheap guards keep other websites out:

- Requests that change something must use `Content-Type: application/json`,
  which browsers will not send cross-site without permission the server never grants.
- The `Host` header must name the server's own address and port, which blocks
  DNS-rebinding access.

Responses are `no-store`, `nosniff`, with a same-origin Content Security Policy.

## Verification

`ctest -L dashboard` covers settings, plan preparation, run control, the API, the
Windows launcher (real processes, including an ltm-compare dry run) and the HTTP server. `node web/tests.mjs` checks the page helpers.
Browser checks are recorded in [reports/dashboard-service.md](../reports/dashboard-service.md).

## Startup check

`long-term-memory-dashboard --plan <reviewed-plan.json> --check-plan` validates
the template and exits before opening a listening socket. The selected saved
settings are validated separately when preparing a run. The default dry-run
configuration uses the standard plan; localmode uses the reviewed compact local
plan and an explicit saved localmode selection.
