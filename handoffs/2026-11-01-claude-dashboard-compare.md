# Claude dashboard connected to ltm-compare

November 1, 2026. Claude (Opus 5.5). Summary, not a transcript.

Dashboard work is on branch claude/thin-dashboard (from codex/dashboard 4f40c15,
main 00c97e0 merged in), pushed. The codex/dashboard checkout was not modified.
Native C++17 service: validated settings, one-at-a-time run control, Windows
Job Object launcher, socket-free API, Boost.Beast server on 127.0.0.1:18080, and
the web page rewired to it. No login and no database, per the user.

Each run copies the reviewed plan (`--plan`, default
configs/pilot-fresh-plan.example.json), fills endpoint_url, reader/judge
{id, version}, provider_policy max_requests/max_tokens/max_cost_nanos and seed,
sets output_directory to "artifacts", writes run-N/config.json and starts
`ltm-compare --config <file> --dry-run`. Other plan fields stay as reviewed.
Dashboard endpoints are now https-only to match ltm-compare.

Observed: Release ARM64 with VS 2026, CTest 56/56 (27 core, 29 dashboard), no /W4
warnings; web checks pass; headless Edge saved a setup and completed a dry run
(2 planned requests, 0 provider requests). Details in docs/DASHBOARD.md and
reports/dashboard-service.md. No paid calls, no services left running.

Open: no execute option is offered; live runs wait for G01 model access and a
user-approved budget. If ltm-compare's config schema changes, update
src/dashboard_plan.cpp and its tests. Phone/LAN access is unverified. Root docs
(README/STATUS/TASKS/DECISIONS) are left to the coordinator.