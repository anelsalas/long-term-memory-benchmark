# Retrieval baselines report

November 1, 2026. Claude (Opus 5.5), branch `claude/retrieval-baselines`.
Method details are in [docs/RETRIEVAL.md](../docs/RETRIEVAL.md). Codex supplied the
pinned dataset, the development ID list and the local embedding server, and fixed
the P02 loader and scorer during this work.

## Question

Is keyword top-1 a fair baseline? This report measures simple retrievers at
matched context budgets: newest-first, BM25, dense embeddings and their RRF
hybrid.

## LongMemEval_S retrieval (60 development questions)

- **Data:** pinned `longmemeval_s_cleaned.json`, revision `98d7416c`, SHA-256
  `d6f21ea9…`; `development-60-seed42.json`, 10 per question type, seed 42.
- **Embedder:** Qwen3-Embedding-0.6B Q8_0, last-token pooling, L2-normalized,
  on llama.cpp `620d2225`. Queries use the instruction prefix; rounds don't.
- **Scoring:** the official retrieval scorer (P04, parity-checked against
  upstream by Codex, main `4cbf8d8`). Abstention questions and questions without
  user-side evidence labels are excluded from retrieval metrics.
- **Recall** below is recall_all: every evidence item retrieved.
- **Budget columns:** the share of questions whose evidence rounds all fit
  within N estimated tokens.

**History time policy include_all** (all 60 load; 46 scorable; later sessions are
flagged per question):

| Method | Turn recall@5 | Turn recall@10 | Turn NDCG@10 | Session recall@5 | All evidence in 1k | 2k | 4k | 8k |
|---|---|---|---|---|---|---|---|---|
| recency | 0.0% | 2.2% | 1.2% | 4.3% | 0.0% | 0.0% | 2.1% | 2.1% |
| bm25 | 60.9% | 73.9% | 66.9% | 67.4% | 31.2% | 54.2% | 66.7% | 81.2% |
| dense | 76.1% | **91.3%** | 77.5% | **89.1%** | 33.3% | 60.4% | **85.4%** | **93.8%** |
| hybrid | **78.3%** | 84.8% | **78.6%** | 82.6% | **37.5%** | **64.6%** | 81.2% | 89.6% |

**History time policy strict** (54 load; 6 with sessions dated after the question
are excluded; 41 scorable):

| Method | Turn recall@5 | Turn recall@10 | Turn NDCG@10 | Session recall@5 | All evidence in 1k | 2k | 4k | 8k |
|---|---|---|---|---|---|---|---|---|
| recency | 0.0% | 2.4% | 1.4% | 4.9% | 0.0% | 0.0% | 2.4% | 2.4% |
| bm25 | 58.5% | 70.7% | 65.7% | 65.9% | 33.3% | 52.4% | 64.3% | 78.6% |
| dense | 75.6% | 90.2% | 78.5% | 87.8% | 33.3% | 61.9% | 85.7% | 92.9% |
| hybrid | 75.6% | 82.9% | 78.2% | 80.5% | 38.1% | 64.3% | 78.6% | 88.1% |

The 54 questions in both tables load identically under both policies, so their
per-question scores are equal. The difference between the tables comes from the
6 added questions, not from the policy changing how a question is scored.
Codex found that their later sessions are 0.19–0.62 days after the question, a
possible timezone effect, and 5 of the 6 have annotated evidence in such a
session.

Observations:
- Dense retrieval beats BM25 by about 15–20 points at every cutoff and at budgets
  of 4k tokens and up.
- The hybrid leads at the top 5 and at small budgets, but RRF with BM25 drags
  dense down at 10 and at large budgets.
- Recency alone almost never covers the evidence. A memory system has to beat
  dense retrieval at a matched budget, not keyword top-1.

## Local Qwen answers on fresh correction histories

- **Setup:** 20 seeds (42–61), 6 distractors, the local Dumas service
  (Qwen3-0.6B Q8_0, 500-token replies, 512-byte prompt cap), $0.
- **Full history** was not run: it exceeds the 512-byte cap.
- **Reading:** stated grades are read with the rule from
  `scripts/report-qwen-replay.ps1`.
- **Record:** `reports/2026-11-01-qwen-budget.json`.

| Condition | Correct grade | Correction in context | Said unknown | Strict integer match |
|---|---|---|---|---|
| Question only | 0/20 | 0/20 | 20 | 0/20 |
| Keyword top-1 | 0/20 | 0/20 | 16 | 0/20 |
| BM25, 50-token budget | **20/20** | 20/20 | 0 | 0/20 |
| Correction alone | 20/20 | 20/20 | 0 | 0/20 |

Strict integer match is 0/20 everywhere because Qwen replies in a sentence; this
matches the earlier runs.

## LongMemEval_S answers with a benchmark-only local Qwen

**Setup**
- **Questions:** the 54 strict development questions (6 are excluded for
  sessions dated after the question).
- **Reader:** Qwen3-0.6B Q8_0 on a benchmark-only llama.cpp server: full
  32,768-token context, thinking on, temperature 0.
  - Built with clang 22.1.3 at `620d222`; build flags and model SHA-256 are
    recorded with the run.
  - Not the Dumas voice service (512-byte prompts, 1024-token context).
- **Conditions:** question only; BM25, dense and hybrid each packed to 6,000
  estimated tokens; the labelled evidence messages only (upper bound). Full
  history (about 115k tokens) does not fit and is reported as `does_not_fit`.
- **Judge:** the official LongMemEval judge prompts (P04), answered by the same
  local Qwen instead of `gpt-4o-2024-08-06`, declared as an adaptation.
- **Record:** `reports/2026-11-01-qwen-longmemeval.json`. It holds IDs,
  verdicts, real token counts and revisions, but no dialogue text; raw answers
  stay in ignored runtime storage.

| Condition | Judged correct | Evidence in reader's context | No answer (thinking loop) | Mean input tokens |
|---|---|---|---|---|
| Question only | 7/54 (13.0%) | 0/51 | 0 | 105 |
| BM25, 6k-token budget | 20/54 (37.0%) | 42/51 | 3 | 6,024 |
| Dense, 6k-token budget | 23/54 (42.6%) | 49/51 | 3 | 6,150 |
| Hybrid, 6k-token budget | **24/54 (44.4%)** | 45/51 | 1 | 6,084 |
| Labelled evidence only | 36/54 (66.7%) | 51/51 | 0 | 332 |

Judged correct by question type (BM25 / dense / hybrid / labelled evidence):

| Question type | BM25 | Dense | Hybrid | Labelled evidence |
|---|---|---|---|---|
| knowledge-update (9) | 4 | 3 | 5 | 7 |
| multi-session (10) | 2 | 2 | 2 | 5 |
| single-session-assistant (10) | 4 | 5 | 5 | 7 |
| single-session-preference (10) | 5 | 2 | 3 | 4 |
| single-session-user (10) | 4 | 6 | 6 | 8 |
| temporal-reasoning (5) | 1 | 2 | 1 | 1 |

**Readings**
- Retrieval puts the evidence in front of the reader most of the time (dense
  49/51). Accuracy still stops near 40%, against 67% with only the evidence. At
  6k tokens, the lower judged accuracy may reflect difficulty using evidence among
  surrounding rounds. Retrieval also misses evidence in some cases. Separating
  these effects requires further experiments and an independently validated judge.
- **No answer:** 7 of 270 cases produced no answer. Greedy decoding made
  Qwen3's thinking repeat until it hit 4,096 output tokens. Qwen's model card
  advises against greedy decoding in thinking mode.

**How the run was assembled**
- The first pass used a 1,024-token cap. Questions whose thinking hit the cap
  were rerun at 4,096. All 22 replies that had finished under 1,024 tokens were
  byte-identical at 4,096, so the larger cap changes only truncated replies.
- An "uncertain" reply stops later requests in the same run, so the 6 questions
  that still looped were rerun with one run per condition.
- Superseded attempts are kept in runtime storage.
- The 5 integer-gold questions ran after Codex's P04 fix (main `b3cdf3d`); each
  case records its `source_revision`.

**Judge check**
- Claude compared a fixed-seed sample of 25 verdicts (5 per condition) with the
  gold answers and replies, and agreed with all 25. Two were borderline: a
  preference rubric, and a self-contradicting reply whose final answer matched.
- This was a check by Claude, not by a human, and the sample is small. The
  seed and per-item audit records are pending, so the reported agreement is
  not independently reproducible from the published artifacts yet.
## Verification

- `ctest -C Release` on Windows ARM64 with VS 2026: 86/86. This covers
  retrieval units, the embeddings client against an in-process Beast server,
  the evaluation tool on the fixture, the budget and public-case CLI checks,
  and Codex's suites.
- Each behavior was written test-first, with the red step observed.
- The dense run was stopped twice by Claude Code's low-memory rule for
  background jobs. With the user's approval it was resumed as an ordinary
  process. The vector cache made each restart lossless.
- One server refusal (HTTP 400) led to the split-and-halve client fix; 1 text
  was truncated in total.
- Lexical runs peak at 442 MB.
- The benchmark Qwen server was stopped after the answer runs.

## Not verified

- The answers were judged by a 0.6B local model, not the official GPT-4o judge.
  Qwen's recommended sampling (instead of greedy) and a stronger reader, for
  example on the idle 12 GB GPU, have not been tried.
- No full-history LongMemEval run: about 115k tokens exceeds Qwen3's 32k
  native context.
- Token budgets are byte estimates, not tokenizer counts. `ltm-compare` also
  bounds prompt bytes by the context's token count, so about a quarter of the
  32k context goes unused.
- One run per configuration; retrieval is deterministic, embedding server
  nondeterminism wasn't measured.
