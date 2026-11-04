# Dashboard service report

Terms are explained in the [glossary](../docs/GLOSSARY.md).

October 31, 2026. Claude (Opus 5.5), branch claude/thin-dashboard from codex/dashboard 4f40c15.

## Scope

Native C++17 service behind the dashboard UI: validated settings, run control,
Windows process launching, a socket-free API and a Boost.Beast HTTP server, plus
the page rewired to it. User direction: a thin control/run/settings panel for
benchmark model tests, with no login and no database.

## Changes to the original UI contract

- Reader and judge each carry provider/id/version (matches the experiment manifest).
- Dataset, condition and case-ID fields were removed: the current command
  (`--offline-demo`) takes no such options. They return when the comparison
  command defines them.
- Results became Output (captured run output). Metric cards return once the
  comparison command writes a results file.
- CSRF tokens, idempotency keys and operation reconciliation were replaced by the
  JSON-only change rule and the Host check described in docs/DASHBOARD.md.

## Development

Behavioral TDD, one failing CTest case per behavior. Two tests passed on first
run because earlier work already covered them: the uint64 overflow cases
(written before their test) and kill-on-drop (Job Object added for the launch
test). The end-to-end offline-demo test connects already-tested parts.

## Observed verification

Windows ARM64, Visual Studio Community 2026 18.11 (CMake 4.3.1, generator
"Visual Studio 18 2026"), Release, Boost 1.92 headers:

- `ctest -C Release`: 48/48 passed. The original 25 core checks plus 23 dashboard
  checks: 6 settings, 5 run control, 4 API, 5 Windows launcher (real processes,
  including the real `--offline-demo`), 2 HTTP server and 1 `--help` smoke check.
  No compiler warnings at /W4.
- `node --check web/app.js` and `node web/tests.mjs`: passed.
- Live server on 127.0.0.1:18080: headers present; foreign Host returned 403;
  form-encoded POST returned 415.
- Headless Edge driven over DevTools: Setup saved with a 18446744073709551615
  token cap and $0.25 (stored as 250000000 nanodollars). Start ran the real
  benchmark to completed (exit 0), and Output showed its report. A second
  dashboard ran a 30-second test command, and Stop in the page ended it as
  stopped. No console errors after adding the page icon. No helper processes remained.

Not verified: phone layout and access over the LAN, and runs of a real
comparison command (not written yet). No model calls were made.

## Comparison command runs

Later the same day, after merging main (00c97e0, `ltm-compare`). Each run now
writes `config.json` from the reviewed plan template plus the saved setup and
starts `ltm-compare --config ... --dry-run`. Endpoints became https-only to match
the command. Behavioral TDD with the red observed for: https-only endpoints,
filling the plan, rejecting a bad template, the prepare hook adding arguments,
a failed preparation leaving nothing behind, and the dry-run hook writing its
configuration. `keeps_template_fields` passed against the stub (it guards against
overwriting), and the real dry-run launch test passed first time (it connects
tested parts).

- `ctest -C Release`: 56/56 passed (27 core including Codex's HTTPS provider and
  CLI checks, 29 dashboard). No /W4 warnings.
- `node --check web/app.js` and `node web/tests.mjs`: passed.
- Headless Edge against 127.0.0.1:18080: setup saved, Start dry run completed
  (exit 0), Output showed `run_kind: dry_run_plan`, 2 planned requests and
  `provider_requests_made: 0`; the run folder held `config.json` and
  `artifacts/run-*/` with plan, manifest and evaluator files. No console errors.

Not verified: an execute run (not offered by the dashboard; needs G01 and a
budget), phone/LAN access. No model calls were made.