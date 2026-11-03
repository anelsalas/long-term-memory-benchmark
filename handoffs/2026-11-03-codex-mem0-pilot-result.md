# Handoff: first Mem0 pilot result, November 3, 2026

Published case `bb7c3b45` completed. GPT judged both Mem0 and the existing dense
baseline correct. Mem0 ingestion made 47 calls over 47 sessions and reported
741,357 input tokens; its reader used 17,363 input tokens versus 10,894 for the
dense baseline. The dialogue-free report and JSON contain measured timing,
resources, provenance and limitations. Raw artifacts remain in ignored runtime.
The dashboard's `mem0-pilot.html` page returned HTTP 200 from the existing server;
dashboard JavaScript tests passed.

Pilot services were stopped after the bridge metrics were saved. The selected ten
multi-session questions contain 490 sessions. Estimate approximately 510 calls
including answers and judgments if one extraction call per session holds; actual
usage can differ. Resume with a shared durable 1,500-call cap for the ten-question
phase, counting the 49 calls already used for the first case. Preserve GPT-only
profiles and existing completed artifacts; await user review before all 54 cases.
