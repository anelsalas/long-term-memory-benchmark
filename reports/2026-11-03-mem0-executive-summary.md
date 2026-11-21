# Experiment report: Mem0 and vector search

Terms are explained in the [glossary](../docs/GLOSSARY.md).

## Executive summary

**Question:** Does Mem0 improve answers over dense vector search for difficult multi-session questions?

**Finding:** GPT judged Mem0 correct on 9 of 10 questions, dense search on 6 of 10, and the labelled-evidence control on 10 of 10.

**Interpretation:** Mem0 is promising for this selected sample. Ten questions do not establish a reliable winner: Mem0 won three paired cases and tied seven; the two-sided sign-test result was p = 0.25.

## Comparison

| Condition | Correct / requested | Reader input tokens, total | Preparation |
| --- | ---: | ---: | --- |
| Mem0 memories | 9/10 | 109,443 | 490 extraction calls, 7,712,359 input and 394,401 output tokens |
| Dense vector search | 6/10 | 108,007 | Embedding preparation cost not recorded in the Mem0 report |
| Labelled evidence | 10/10 | Not reported here | Benchmark-provided evidence |

All ten Mem0 answers were completed and judged. The paired dense and labelled-evidence results were reused from an earlier run; run-to-run variance is unmeasured. Each reader had a 6,000 estimated-token context budget, and GPT was both reader and judge.

## Workload and setup

Ten selected LongMemEval-S multi-session questions, including an abstention case, were drawn from the development set. Mem0 3.3.1 used GPT at low effort to extract facts, a local Qwen3 embedding model and a SQLite-backed memory store. GPT at high effort answered and graded. Local orchestration and storage ran on the Surface; model calls used the configured GPT connection. Exact versions, question IDs and source references are in the [detailed report](2026-11-03-mem0-phase2.md).

## Measurements

Mem0 ingestion took about **8 hours 10 minutes** summed over sessions, excluding gaps. Reader input averaged about **11,000 tokens per question**, while preparation averaged about **771,000 input tokens per history**. An estimated 2.4 million preparation input tokens were Codex CLI framing; this is not a measured cost decomposition. Actual monetary cost and a controlled retrieval-latency comparison were not measured.

Memory sampling covered part of the first ingestion: approximately 1.02 GiB peak working set for the local embedding process and 346 MiB for the Mem0 sidecar. It did not cover the complete run or remote inference. The [JSON record](2026-11-03-mem0-phase2.json) includes per-question usage and timings.

## Reliability and limitations

The selected Mem0 memories referenced every labelled **session** in each case; this does not prove that every labelled message was recovered. The dataset is public, the ten cases were selected where dense retrieval had difficulty, and GPT graded the answers. The 9/10 and 6/10 scores are model judgments on a small sample, not a general performance guarantee.

## Decision and next step

Keep Mem0 as a candidate and compare its preparation cost and accuracy with lighter alternatives. A useful future experiment is a matched Surface comparison of vector scans and indexed search, recording accuracy, latency and resource use as the memory collection grows. The user has requested a stop to further model calls until a new experiment is chosen.
