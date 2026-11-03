# Handoff: Mem0 local resource sampling, November 3, 2026

`scripts/sample-mem0-resources.ps1` samples the local embedding server, bridge,
sidecar and ingestion runner every five seconds. The running pilot's ignored
`runtime/mem0-pilot/monitor.json` and `resource-samples.jsonl` track the sampler.
Sampling started after ingestion began and excludes GPT subprocesses and remote
inference; report timestamps, process scope and sampled peaks accordingly.

`report.js` accepts `--resources` after the progress files. Node offline tests
passed 13/13. The 47-session GPT-only case remains active with a durable cap of
150 attempts. Check runner and sampler state before taking action; compare and
publish a dialogue-free report after the case and judge complete.
