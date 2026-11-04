# Explicit local dashboard execution

Terms are explained in the [glossary](../docs/GLOSSARY.md).

November 1, 2026. Isolated `codex/local-dashboard` checkout; existing audio and
Claude's source checkout preserved. Implementation covers settings, plan
preparation and the existing browser UI/API.

Settings persist `run_mode`, defaulting to `dry_run` for older files. Explicit
`local` mode validates numeric loopback HTTP, positive request/token caps and
zero cost. Local plan preparation requires a reviewed compact profile with
verified token bounds, selects `execution_mode:"local"`, removes API-key
configuration and zeros all prices. The saved local mode chooses `--execute`;
dry planning retains `--dry-run`. The Setup selector and Control button show
the saved action clearly; local cost is fixed at zero in UI and backend.

## Observed verification

- Behavioral RED before implementation: `endpoint_rules` and `fills_settings`
  rejected `run_mode` as unexpected.
- Native Windows ARM64 Release, VS2022/MSVC19.44: scoped settings/plan CTest
  **10/10 passed** after implementation, including legacy settings, exact local
  host restrictions, zero-cost/positive-cap enforcement, local price/key
  isolation, trusted-template requirement and execute/dry-run arguments.
- `node web/tests.mjs` passed: mode labels, local-cost validation, Setup mode
  control, existing integer/cost conversions and API behavior.
- Initial sandbox CTest exposed the known temporary-file replacement permission
  restriction; normal host execution passed the same file-store test. A test
  fixture initially duplicated `run_mode` after serialization gained that field;
  the fixture now replaces the field and the endpoint checks pass.

The coordinator supplies the reviewed local plan, integrated executable build,
browser acceptance and actual model execution. This worker made zero model
calls, launched no services and changed no audio settings. The coordinator's
earlier two-call CLI result remains full-history correct/retrieved wrong;
this UI change supplies execution controls rather than a new accuracy claim.

## Resume

Build the integrated dashboard and `ltm-compare`; start it with the reviewed
`configs/local-qwen-smoke.example.json` plan. Save the explicit local setup and
verify the browser Run action, artifacts and reported actual answers. Existing
HTTPS dry planning remains covered. A compact local run is a declared pilot
adaptation, separate from full published-benchmark reproduction.

## Coordinator browser acceptance

November1,2026. Integrated nativeVS2026 ARM64 build passed57/57 CTest.
Coordinator caught an additional main-program startup failure: validating the
reviewed template via an empty Settings object rejected endpoint_url. Observed
serverexit1 before the fix. Separated template validation from populated saved
settings, added --check-plan and the startup-check regression; it passes.

Started the new loopback server, saved explicit localmode via the actual browser,
clicked Startlocalrun and observed run-2 complete exit0. Output records
measured_comparison, provider_requests_made2, cost0, graded2 andaccuracy0.5.
The repeated localfixture again answered82(full correct) and23(retrieved wrong).
The earlier browser dryrun remains preserved as run-1. This repeats one synthetic
question rather than adding independent research samples. Screenshot proof and
full artifacts remain in ignored dashboard-acceptance runtime storage.

The browser was tested in its narrow panel layout. Physical cellphone/LAN access
remains a separate device/network check. Claude originalcheckout was preserved.
