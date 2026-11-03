# Summary: first Mem0 pilot result, November 3, 2026

Completed the 47-session GPT-only Mem0 pilot and the matched single-question
reader/judge comparison. GPT judged Mem0 and the previously recorded dense
baseline correct. Mem0 ingestion used 47 calls and 741,357 reported input tokens;
reader input was 17,363 versus the dense baseline's 10,894. See the dialogue-free
pilot report for resource measurements, timing conditions and limitations.

The phone-friendly dashboard result page is linked from the main dashboard and
returned HTTP 200 from the existing server. Dashboard JavaScript tests passed.

The ten selected questions contain 490 sessions, suggesting approximately 510
calls including reading and judging if the one-call-per-session pattern holds.
The next phase still requires a shared durable 1,500-call cap and measured usage.
Pilot services were stopped after final counters were captured.
