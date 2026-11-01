# Codex local Qwen execution handoff

November1,2026. Summary, not a transcript.

User requested actual runs and dashboard verification. Original Claude checkout
Source/ltm-thin-dashboard at20a6d91 passed56/56 native CTest; webchecks passed.
Started its server PID21836 on127.0.0.1:18080 with isolated runtime/dashboard-acceptance
state. Browser SaveSetup/Start/Output completed dryrun exit0. Screenshot retained
runtime/dashboard-acceptance/browser-completed.png; tab marked deliverable.

Added explicit core local_mode/execution_mode and compactprofile. Nativecore27/27
passed. Two actual Qwen requests at127.0.0.1:8766 returned82(full correct) and23
(top1 retrieval incorrect), gold82. Rootruntime/local-qwen-measured.json config,
artifacts under runtime/local-qwen-measured/run-17935475397914087-0. Public report
contains only synthetic machine-independent evidence. No paidcalls/settingschanges.

Dashboard localrun work is active in isolated build-ltm-local-dashboard branch
codex/local-dashboard based onClaude20a6d91. Research worker owns dashboard settings,
plan/UI/tests/docs; root copied corelocal files and configured VS2026 ARM64 build.
Integrate tests, launch updated server after stopping only owned PID21836, save
explicit localsetup, clicklocalrun and verify actualresponse summary inbrowser.
Preserve Claude originalcheckout and unrelated Dumas audio changes. Current model
service may reportstop unconditionally; recordlimits. Pending publishedlonghistory
reproduction needs largercontextprofile. Root owns shared docs/commitpublication.
