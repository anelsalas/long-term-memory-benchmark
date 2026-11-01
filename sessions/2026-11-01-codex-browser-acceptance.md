# Codex session summary

November1,2026. Summary, not a transcript.

Active integration checkout build-ltm-local-dashboard branch codex/local-dashboard
merges Claude20a6d91 and mainc01565a plus localcontrol1f00217. OriginalClaude
Source/ltm-thin-dashboard checkout preserved. IntegratednativeVS2026ARM64 build
passed57/57 CTest; Nodewebchecks passed. Startupbug fixed via separated template
validation and --check-plan regression. Browserexplicitlocal mode and Startlocalrun
completedrun-2 exit0,2 actual Qwen requests, cost0,accuracy0.5. Fullanswer82correct,
retrieved23wrong; the keywordreference missed thecorrection. OriginalCLIrun was
also2requests withsameanswers; this is a repeated one-question smoke.

Dashboard PID14676 is intentionally running at127.0.0.1:18080. Executable:
build-ltm-local-dashboard/build-vs2026/Release/long-term-memory-dashboard.exe.
Data: build-ltm/runtime/dashboard-acceptance. Plan: build-ltm/runtime/local-qwen-measured.json.
Browsermarked deliverable at/#output. Screenshot browser-local-measured.png inthat
runtimedirectory. Modelresidentservice PID26240 at127.0.0.1:8766 preserved.
Noother taskcommand remainsrunning. NoAPIspending/modelsettings/audiochanges.

Allcode and reviewedsyntheticresults can publish; runtime/privateartifacts stay
ignored. Sourcebounds512bytes/32tokens drovecompactadaptation, notpaperreproduction.
Reportedcompletiontokens2peranswer, inputunknown;1088tokens/run are reservations.
Runnerresourcesexcludemodelworker. Largerfrozenmemorytests, officialretrievalranking
adapter, matchedtimings andphysicalphone/LAN acceptance remainnext. Keep failing
baselineevidence. Dumasconsumerpinupgrade remainsseparate fromaudio work.
