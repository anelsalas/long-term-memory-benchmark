# Dashboard

A thin local control panel for benchmark runs: save the model setup and limits,
start and stop runs, and read their output. [ltm-compare](COMPARE.md) does the
work; the dashboard prepares its configuration and launches it as a dry run.

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
`COMMAND --config <run folder>/config.json --dry-run`. Every other plan field
(source revision, credential variable name, prices, token limits, top-k,
timeouts, public case) stays as reviewed in the plan file. A dry run makes zero
provider calls and reads no credentials. The dashboard has no execute option;
live runs need G01's model access and an approved budget.
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

- `endpoint_url`: the full https chat-completions URL; no credentials, query or fragment.
- `reader` and `judge`: nonblank `provider`, `id` and `version`. The plan receives
  `id` and `version` (the exact model sent on the wire); `provider` is recorded with the run.
- `max_requests`, `max_tokens`, `max_cost_nanos` (USD × 10^9) and `seed`: uint64
  decimal strings.

Model credentials are never entered in the dashboard.

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
