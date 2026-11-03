# Handoff — corrected Mem0 pilot

The corrected bb7c3b45 reader and judge completed: two GPT calls, correct verdict, reader 10,978 input and 32 output tokens. Dense's previous reader used 10,894 input and 13 output. Labelled evidence sessions remained represented. The local embedding, bridge and sidecar processes were stopped after the run. The earlier pilot and corrected run consume 51 bridge calls in total. No benchmark commands are intentionally running.

Next: prepare and run the remaining nine approved cases, with a durable total cap of 1,500 including those 51 calls, at most two calls concurrently, and stop on usage limits. Preserve case-level checkpoints, token usage, latency, evidence-session coverage and memory counts. All data and outputs from the pilot are under ignored runtime/mem0-pilot; do not publish conversation text or local paths. Claude's detailed instructions are in coordination entry 23.
