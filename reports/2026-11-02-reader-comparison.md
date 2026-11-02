# Reader comparison on LongMemEval_S

November 2, 2026. Claude (Opus 5.5), branch `claude/stronger-readers`. Record:
[2026-11-02-reader-comparison.json](2026-11-02-reader-comparison.json), which holds
verdicts, token counts and timings, without dialogue text.

## Question

The local Qwen3-0.6B run left this open: how much of LongMemEval accuracy depends
on the reader rather than on retrieval? This run keeps everything else fixed and
changes only the reader:
- the 54 strict development questions;
- the five conditions;
- the packed evidence.

## Setup

**Readers**
- **Qwen3-0.6B Q8_0.** The saved answers from
  [the earlier run](2026-11-01-retrieval-baselines.md), not regenerated.
- **Claude Opus 5.5 (`claude-opus-5-5`), effort high,** via the owner's signed-in
  Claude Code 2.1.316.
- **GPT-6.1-sol, reasoning effort high,** via the owner's signed-in Codex CLI
  0.163.0.
- **How the CLIs ran.** Both ran through `ltm-cli-bridge`
  ([docs/CLI-BRIDGE.md](../docs/CLI-BRIDGE.md)): tools off, the same neutral system
  prompt, an empty working folder. The plans' usage is not billed per request: $0
  marginal cost.

**Conditions** (unchanged from the Qwen run)
- question only;
- BM25, dense and hybrid rankings, each packed to 6,000 estimated tokens of user
  rounds;
- the labelled evidence messages only.

**Same evidence for every reader.** The packed source IDs are identical across
all three readers for all 270 question-condition pairs.

**Judges**
- **Primary: GPT-6.1-sol, reasoning effort high.** The official LongMemEval judge
  prompts (P04) and P04's strict yes/no parser. The user chose the judge at the
  highest effort.
  - Claude and GPT answers were judged during their runs.
  - Qwen's answers were re-judged with `ltm-rejudge`, which sends the saved judge
    prompts byte for byte.
- **Second judge: Claude Opus 5.5, effort high,** on every answer, for agreement.
- **Adaptations.** Neither judge is the official `gpt-4o-2024-08-06`. The CLIs
  don't apply temperature, so the judge's temperature 0 isn't applied.

## Results

Judged correct by GPT-6.1-sol, out of 54 questions:

| Condition | Qwen3-0.6B | Claude Opus 5.5 | GPT-6.1-sol | Evidence in context |
|---|---|---|---|---|
| Question only | 5 (9.3%) | 4 (7.4%) | 4 (7.4%) | 0/51 |
| BM25, 6k budget | 18 (33.3%) | 44 (81.5%) | 41 (75.9%) | 42/51 |
| Dense, 6k budget | 19 (35.2%) | **48 (88.9%)** | **46 (85.2%)** | 49/51 |
| Hybrid, 6k budget | 22 (40.7%) | 47 (87.0%) | 45 (83.3%) | 45/51 |
| Labelled evidence only | 36 (66.7%) | 51 (94.4%) | 50 (92.6%) | 51/51 |

"Evidence in context" counts questions where at least one labelled evidence
message was packed; it is the same for every reader. Three abstention questions
have no labels.

Judged correct by question type, dense retrieval / labelled evidence:

| Question type | Qwen3-0.6B | Claude Opus 5.5 | GPT-6.1-sol |
|---|---|---|---|
| knowledge-update (9) | 3 / 7 | 9 / 8 | 8 / 8 |
| multi-session (10) | 1 / 4 | 6 / 10 | 6 / 10 |
| single-session-assistant (10) | 6 / 8 | 8 / 9 | 8 / 9 |
| single-session-preference (10) | 2 / 6 | 10 / 10 | 9 / 9 |
| single-session-user (10) | 6 / 8 | 10 / 9 | 10 / 9 |
| temporal-reasoning (5) | 1 / 3 | 5 / 5 | 5 / 5 |

Reader cost per answer (means):

| Reader | Seconds, 6k conditions | Input tokens, dense condition |
|---|---|---|
| Qwen3-0.6B (local CPU) | 40–44 | 6,150 |
| Claude Opus 5.5 | 3.0 | 10,056 |
| GPT-6.1-sol | 5.6–6.8 | 11,041 |

Input tokens are each model's own count, including the CLI's framing. Codex adds
about 5,000 tokens and Claude Code about 700. Strong readers' tokenizers also
count the same prompt as more tokens than the byte estimate assumes.

## Judge agreement

Both strong judges graded every answer, including answers from their own model
family:

| Answers by | GPT-6.1-sol vs Claude Opus 5.5 judge | Qwen judge vs GPT judge |
|---|---|---|
| Qwen3-0.6B | 260/263 agree | 241/263 agree |
| Claude Opus 5.5 | 269/270 agree | — |
| GPT-6.1-sol | 268/270 agree | — |

Under the Claude judge, every count in the results table is the same, except:
- Claude Opus 5.5 with labelled evidence: 50 rather than 51;
- Qwen: BM25 19, dense 18, labelled evidence 35.

All three disagreements on the strong readers' answers are
single-session-preference questions in the labelled-evidence condition, where
the rubric is open to judgment. Neither judge favors its own family's answers:
- On Claude's answers, the only disagreement is the Claude judge rejecting one.
- On GPT's answers, there is one flip in each direction.

The 7 Qwen answers without a reply are counted as not correct and were not sent
to any judge.

## Readings

- **The reader dominates at this budget.** With the same evidence, accuracy goes:
  - dense: 35% (Qwen) to 85–89%;
  - labelled evidence: 67% to 93–94%.
- **Retrieval's remaining gap is small for strong readers.** Dense retrieval at
  6k tokens is 5–7 points below labelled evidence.
- **The gap is mostly multi-session questions.** There, both strong readers get
  10 of 10 with the evidence and 6 of 10 with dense retrieval. Evidence spread
  across sessions is where a memory system can still add accuracy over plain
  retrieval.
- **Ranking of retrievers.** Dense beats BM25 by 4–7 points for strong readers,
  and the hybrid doesn't beat dense. That matches the retrieval metrics.
- **Qwen's own judging was lenient.** On its answers, Qwen-as-judge agreed with
  the GPT judge 241 of 263 times and accepted more answers than either strong
  judge. Under the GPT judge, Qwen's figures fall from 13.0 / 37.0 / 42.6 / 44.4 /
  66.7% to 9.3 / 33.3 / 35.2 / 40.7 / 66.7%.
- **The strong judges agree with each other far more.** On Qwen's answers, GPT
  and Claude agree 260 of 263 times. Across all three readers, they agree on
  797 of 803 judged answers.

## How the run was assembled

- **Workers.** Each reader ran as three parallel workers, one `ltm-compare` run
  per question with all five conditions.
- **Incomplete runs.** A run with any failed request is incomplete. Such runs
  were set aside whole (kept in runtime storage) and rerun in full. The failures
  came from infrastructure, not from answer content:
  - **A bridge parser bug.** Codex prints "Reconnecting…" error events after a
    Cloudflare 403 on its WebSocket, then falls back to HTTPS and answers. The
    bridge first counted those as failures. That left 19 Claude-read and
    27 GPT-read questions incomplete. Fixed test-first; Codex calls have not
    failed since.
  - **Restarts.** Two bridge restarts to install fixes interrupted in-flight
    requests.
  - **The Claude plan's usage limit.** It was reached mid-rerun; 6 questions
    were rerun after it reset.
- **Retries.** After the fix, the bridge retries a run that ends without an
  answer up to twice. The only logged failures in the final runs are the
  usage-limit exits, whose questions were rerun.
- **Final runs.** All 54 questions × 5 conditions completed and were judged for
  both strong readers: 270 answers each, none missing.

## Not verified

- **Judges.** Neither judge is the official GPT-4o, and agreement with a human
  grader is unmeasured. The two strong judges agree on 797 of 803 judged
  answers, but they could share the same mistakes.
- **One run per configuration.** Model sampling can't be set through the CLIs;
  run-to-run variance is unmeasured.
- **Full history (~115k tokens) was not run.** It fits both strong models'
  contexts, but costs about 6M input tokens per reader. That is the next
  long-context comparison, if the plans allow.
- **Framing.** The CLIs add their own framing (Codex's tool schemas and base
  instructions) even with tools disabled. Readers are not compared on identical
  raw prompts, only on identical evidence and instructions.
