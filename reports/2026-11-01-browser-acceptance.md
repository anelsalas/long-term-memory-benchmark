# Browser and local model acceptance

November1,2026. Codex coordinator observed actual browser state and native tests.

- Original Claude20a6d91 checkout:56/56 native tests passed; Node webchecks passed.
- Browser SaveSetup/Startdryrun/Output: completed exit0,2planned,0providerrequests.
- Isolated localdashboard integration:57/57 native tests passed after startup fix.
- Browser saved explicit localmode/Qwen model,2requestcap,2048tokencap andcost0.
- Clicked Startlocalrun: run-2 completed exit0 with2 actual localrequests.
- Output showed graded2,accuracy0.5,cost0 andsaved measured_comparison artifacts.

The fixture is Lina's score23 corrected to82, with an unrelated person's63.
Fullcontext answered82 correctly; top1keyword retrieval selected the old source
and answered23. This repeated the earlier CLI smoke result. The infrastructure
checks passed while the baseline retrieval answer failed; both are preserved.
See main reports/2026-11-01-local-qwen-smoke.md for first-run numeric evidence.

The local service's512-byte/32-output-token caps required a declared compact
profile. Inputtoken counts remain unavailable; retainedreservations remain
separate from measured completiontokens. RunnerCPU/RAM excludes modelworker
resources. Timings in the original run have unmatched cold/warm state.

Screenshots/fullrun outputs are kept in ignored runtime/dashboard-acceptance.
The active page is http://127.0.0.1:18080/. The server is loopback-only. Narrow
browser layout verified; physicalphone/LAN access remains unverified. Existing
Claude sourcecheckout and Dumas audio settings were preserved.
