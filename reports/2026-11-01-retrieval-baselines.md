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

## Verification

- `ctest -C Release` on Windows ARM64 with VS 2026: 84/84. This covers
  retrieval units, the embeddings client against an in-process Beast server,
  the evaluation tool on the fixture, the `bm25_budget` and public-case CLI
  checks, and Codex's suites.
- Each behavior was written test-first, with the red step observed.
- The dense run was stopped twice by Claude Code's low-memory rule for
  background jobs. With the user's approval it was resumed as an ordinary
  process. The vector cache made each restart lossless.
- One server refusal (HTTP 400) led to the split-and-halve client fix; 1 text
  was truncated in total.
- Lexical runs peak at 442 MB.

## Not verified

- No LongMemEval answers have been produced or judged yet; that needs the
  benchmark-only Qwen server (see the scripts).
- No full-history LongMemEval run: about 115k tokens exceeds Qwen3's 32k
  native context.
- Dense and hybrid aren't yet available as `ltm-compare` answer conditions.
- Token budgets are byte estimates, not tokenizer counts.
- One run per configuration; retrieval is deterministic, embedding server
  nondeterminism wasn't measured.
