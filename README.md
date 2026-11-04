# Long-Term Memory Benchmark

Measure and improve reliable long-term memory access on everyday computers.

[Pending tasks](TASKS.md) · [Current status](STATUS.md) · [Research protocol](research/PROTOCOL.md)

Compare memory methods by answer accuracy, evidence retrieval, latency, token
usage and cost. Build native C++17 components for Windows ARM64, with
CMake targets and automated tests that applications can consume as a pinned dependency.

## Latest result: Mem0 (November 3, 2026)

[Mem0 report](reports/2026-11-03-mem0-phase2.md) and its
[per-question record](reports/2026-11-03-mem0-phase2.json)

Mem0 3.3.1 (open source) was run on the 10 multi-session questions, where dense
retrieval lost the most in the run below. Mem0 and dense used the same reader and
judge, GPT-6.1-sol at high effort, and the same 6,000-token context budget. The
reader's input across the ten was 109,443 tokens for Mem0 and 108,007 for dense.

| Reader's context (GPT-6.1-sol) | Correct, out of 10 |
|---|---|
| Dense embeddings | 6 |
| **Mem0** | **9** |
| Labelled evidence only (control) | 10 |

- **Mem0 vs dense.** Mem0 won 3, lost 0 and tied 7. A two-sided sign test gives
  p = 0.25, so ten questions can't separate the two methods.
- **Ingestion cost.** Mem0 reads each history before it can answer:
  - **Total:** 490 GPT-6.1-sol calls at low effort, with 7.7M input and 0.39M
    output tokens, about 8 hours 10 minutes for the ten histories.
  - **Per question:** about 771k ingestion input tokens, against about 11k to
    answer.
  - **CLI framing:** an estimated 2.4M of the input tokens is framing the Codex
    CLI adds to every call.
- **Caveats.**
  - The dense and labelled-evidence results are reused from the November 2 run,
    not rerun beside Mem0, so run-to-run variance is unmeasured.
  - GPT is both reader and judge.
  - Evidence presence is checked at the session level, not the turn level.
  - No Claude model was called.

## Latest results (November 2, 2026)

Full reports:
- [Reader comparison](reports/2026-11-02-reader-comparison.md) and its
  [per-answer record](reports/2026-11-02-reader-comparison.json)
- [Retrieval baselines and the local Qwen run](reports/2026-11-01-retrieval-baselines.md)

**Setup**
- **Questions.** 54 development questions from LongMemEval_S, pinned revision
  `98d7416c`, from a stratified 60-question selection. Six cases with later-dated
  sessions are excluded under the strict policy.
- **Context.** The selected source IDs match across readers for each condition.
  The serving stacks and prompt framing differ. Ranked
  conversation rounds are packed into 6,000 estimated tokens and shown in date
  order.
- **Readers.**
  - Qwen3-0.6B runs locally on the CPU.
  - Claude Opus 5.5 and GPT-6.1-sol run at high effort through the owner's
    signed-in Claude Code and Codex CLIs, with tools disabled.
- **Judging.** The official LongMemEval judge prompts, answered by GPT-6.1-sol at
  high effort.

**Answers judged correct, out of 54**

| Reader's context | Qwen3-0.6B | Claude Opus 5.5 | GPT-6.1-sol | Evidence present |
|---|---|---|---|---|
| Question only | 9.3% | 7.4% | 7.4% | 0/51 |
| BM25 | 33.3% | 81.5% | 75.9% | 42/51 |
| Dense embeddings | 35.2% | **88.9%** | **85.2%** | 49/51 |
| BM25 + dense (RRF) | 40.7% | 87.0% | 83.3% | 45/51 |
| Labelled evidence only (control) | 66.7% | 94.4% | 92.6% | 51/51 |

**Retrieval alone.** Share of questions with every evidence turn retrieved, 46
scorable questions, official LongMemEval retrieval metrics:

| Method | Top 5 | Top 10 |
|---|---|---|
| Newest first | 0.0% | 2.2% |
| BM25 | 60.9% | 73.9% |
| Dense embeddings (Qwen3-Embedding-0.6B) | 76.1% | **91.3%** |
| BM25 + dense (RRF) | **78.3%** | 84.8% |

**Findings**
- **Reader results differ.** With matching selected source IDs, dense retrieval
  scores 35% with Qwen3-0.6B and 85–89% with the stronger readers. Serving
  configuration and prompt framing also differ.
- **Retrieval's remaining gap is multi-session questions.** For strong readers,
  dense retrieval is 5–7 points below the labelled evidence. That gap is mostly
  multi-session questions: 6 of 10 with retrieval, against 10 of 10 with the
  evidence. A memory system has to beat dense retrieval there, at a matched
  budget.
- **The judges are consistent.** A second judge, Claude Opus 5.5, agrees with
  the GPT judge on 797 of 803 answers. Agreement measures consistency; shared
  errors and bias remain possible.
  - Qwen3-0.6B accepted more of its own answers than the GPT judge.

**Limits**
- One run per configuration; the sample is small.
- Actual subscription-credit expenditure was not measured; requests may use
  included quota or additional paid credits.
- The judges aren't the official GPT-4o, and no human has graded the answers.
- Token budgets are byte estimates.
- Full history (~115k tokens) and Graphiti haven't been run on these questions
  yet. Mem0 has run only on the 10 multi-session questions (above).

## Run

The current executable runs synthetic lexical-retrieval component checks and
prints source coverage, timing, CPU time and process-memory samples as JSON.
The paired comparison command prepares fresh/published cases and executes
configured HTTPS model requests under declared budgets.

```powershell
cmake -S . -B build -G "Visual Studio 17 2022" -A ARM64
cmake --build build --config Release
ctest --test-dir build -C Release --output-on-failure
```

Run `build/Release/long-term-memory-benchmark --offline-demo`.

Library modules provide [experiment manifests and results](docs/EXPERIMENT.md)
and a [LongMemEval dataset adapter](docs/LONGMEMEVAL.md).

## Paired comparison

```powershell
build/Release/ltm-compare --config configs/pilot-fresh-plan.example.json --dry-run
```

Dry-run saves full-history/retrieved-evidence plans with zero model calls.
[Command and artifacts](docs/COMPARE.md) define dashboard integration and execute
configuration. [Model access](research/MODEL-ACCESS.md) records the current shortlist
and proposed pilot envelope. Example configs have zero spending allowances.

## Local model smoke

The current local Qwen profile allows a 500-token reply. Use
`configs/local-qwen-500.example.json` for a new local run. The [matched replay
report](reports/2026-11-01-qwen-500-replay.md) compares the same model and
histories at both reply limits. Open **Results** on the dashboard to read each answer.

With the reviewed local service running:

```powershell
build/Release/ltm-compare --config configs/local-qwen-smoke.example.json --execute
```

The explicit local profile uses numeric loopback, zero APIprices and bounded
requests. [First measured result](reports/2026-11-01-local-qwen-smoke.md): full
history answered correctly; lexical top1 missed the correction.

## Research foundation

- [LongMemEval: Benchmarking Chat Assistants on Long-Term Interactive Memory](https://arxiv.org/abs/2410.10813): memory evaluation and retrieval design.
- [Mem0: Building Production-Ready AI Agents with Scalable Long-Term Memory](https://arxiv.org/abs/2504.19413): fact extraction, updates and efficiency.
- [Zep: A Temporal Knowledge Graph Architecture for Agent Memory](https://arxiv.org/abs/2501.13956): temporal relationships and evidence.

[Review and continuation guide](REVIEW-START-HERE.md) summarizes implementation,
verified tests, limitations and resume steps for another model.

## Develop and evaluate

Use [the objective](PROJECT.md), [research protocol](research/PROTOCOL.md),
[source attribution](research/SOURCES.md), [Application integration](docs/INTEGRATION.md),
and [current progress](STATUS.md). Research ownership instructions are in
[research/RESEARCH-OWNER.md](research/RESEARCH-OWNER.md).

Fast component tests run on code changes. Selected integration tests exercise
memory adapters. Full model experiments use explicit configurations and budgets.
Public results identify dataset, model, method, scorer and execution versions.

[Resource measurements](docs/RESOURCES.md) define CPU time, sampled memory,
process-lifetime peak memory and stage timing for fair efficiency comparisons.

License: [Apache-2.0](LICENSE).

## Dashboard

See [dashboard continuation](DASHBOARD-START-HERE.md) and
[UI/API contract](docs/DASHBOARD.md) for the lightweight Surface/cellphone interface.

[Sonnet and Qwen on the first correction case](reports/2026-11-01-sonnet-first-case.json) records the four matched prompts and actual replies.
