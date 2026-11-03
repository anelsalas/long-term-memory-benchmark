# Handoff: Mem0 single-case report tool, November 3, 2026

`adapters/mem0/report.js` generates a dialogue-free comparison from completed
progress JSONL, `ltm-compare` result artifacts and the existing reader comparison
report. Tests cover deduplication, missing cases and absence of dialogue. The
summed session client time excludes gaps and selects the longest observed time
for each replayed session. Node offline tests passed 13/13.

The live GPT-only 47-session case remains running under a 150-attempt durable
cap. Check the ignored pilot runner and progress before any action. After it
finishes, use the prepared comparison config and report script; preserve local
raw artifacts and publish only the dialogue-free report after validation.
