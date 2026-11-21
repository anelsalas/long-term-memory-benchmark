# Experiment report: [title]

Terms are explained in the [glossary](../docs/GLOSSARY.md). Copy this template for each experiment and link the detailed results. Use “not measured” or “not applicable” where appropriate; identify estimates and their assumptions.

## Executive summary

**Question:** [What decision does this experiment support?]

**Finding:** [Result in one or two sentences, with sample size.]

**Interpretation:** [What the result supports and its most important limitation.]

## Comparison

| Condition | Purpose | Correct / requested | Errors / unanswered | Evidence found | Reader input tokens | Retrieval time | Preparation cost |
| --- | --- | ---: | ---: | --- | ---: | --- | --- |
| [Method] | [What it tests] | [x/n] | [x/n] | [definition, x/n] | [total or per question] | [p50/p95, repetitions] | [time, tokens, or not measured] |
| [Baseline] | [Reference] | [x/n] | [x/n] | [definition, x/n] | [total or per question] | [p50/p95, repetitions] | [time, tokens, or not measured] |

**Comparison rules:** [Shared questions, reader, judge, context budget and preprocessing; differences and reused results.]

## Workload and setup

- **Dataset:** [name, version, checksum, question IDs, selection and exclusions].
- **Methods:** [versions, storage, index, extraction, update and retrieval configuration].
- **Models:** [extraction, embeddings, reader and judge; versions, settings and where each runs].
- **Hardware:** [device, OS, CPU/GPU/RAM, concurrency and relevant background load].
- **Run:** [date, code commit, prompt/config versions, context and output limits, cache state, repetitions].

## Measurements

| Stage | Time and scope | Input / output tokens | Calls and failures | Local CPU / RAM / storage | Actual cost |
| --- | --- | --- | --- | --- | --- |
| Preparation / ingestion | [one-time vs incremental; measured window] | [input / output] | [calls, retries, missing usage] | [scope, sampled peak] | [amount or not measured] |
| Query embedding and retrieval | [cold/warm; p50/p95; repetitions] | [input / output or not applicable] | [calls, failures] | [scope] | [amount or not measured] |
| Answer generation | [p50/p95; includes network?] | [input / output] | [calls, failures] | [scope] | [amount or not measured] |
| Grading | [time or not measured] | [input / output] | [calls, failures] | [scope] | [amount or not measured] |

State whether token counts include provider or CLI framing, which costs are estimates, and whether resource measurements cover the runner, workers, remote inference or only selected processes. Record index size and build time when measured.

## Reliability and limitations

- **Coverage:** [requested, completed, judged, failed and excluded cases; category results and denominators].
- **Evidence:** [any/all evidence; turn or session level; provenance completeness; exclusions].
- **Fairness:** [matched vs reused runs, truncation, adaptations, uncertainty and paired differences].
- **Validation:** [judge identity, independent or human checks, contamination controls and held-out data].
- **Limits:** [sample size, missing measures and conclusions the data can support].

## Decision and next step

**Decision:** [What should be kept, changed or investigated?]

**Next experiment:** [Question, comparison and success measure.]

**Artifacts:** [Dialogue-free results, code and configuration revisions, detailed report and upstream sources.]
