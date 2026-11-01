# Retrieval baselines

Simple retrievers that every memory system should beat, compared under the same
context budget. They follow LongMemEval's finding that conversation rounds are
the right retrieval unit.

## Methods

| Method | What it ranks |
| --- | --- |
| `recency` | Newest round first. Shows what a budget alone achieves. |
| `bm25` | Okapi BM25 over round text: k1 = 1.2, b = 0.75, IDF = ln(1 + (N − n + 0.5) / (n + 0.5)). Terms are lowercase ASCII letters and digits; UTF-8 bytes stay inside terms. No stemming and no stop words. |
| `dense` | Cosine similarity between embeddings of the question and of each round. The question gets an instruction prefix; rounds get none. |
| `hybrid` | Reciprocal rank fusion of `bm25` and `dense`, k = 60. |

A **round** is one user message plus the replies that follow it, identified by
the user message's source ID. LongMemEval's official retrieval corpus is user
messages only, so ranking scores the user text. The budget counts the whole
round, user message plus replies.

The **budget packer** takes the longest prefix of a ranking whose rounds fit the
token budget, stopping at the first round that doesn't fit. It presents the
kept rounds in chronological order. Tokens are estimated as ceil(UTF-8 bytes / 4)
for every method, so budgets are matched; the estimate is not any model's
tokenizer.

Code: `include/long_term_memory_benchmark/retrieval.hpp` and the matching
sources. Behavior is covered by `ctest -L retrieval`.

## LongMemEval_S evaluation

```powershell
build-vs2026\Release\ltm-retrieval-eval.exe --dataset longmemeval_s_cleaned.json `
  --provenance provenance.json --questions development-60-seed42.json --out results.json `
  [--budgets 1000,2000,4000,8000] `
  [--embeddings-url http://127.0.0.1:8771/v1/embeddings --embedding-model NAME --embedding-cache DIR]
```

- The tool verifies the dataset's size and SHA-256 against `provenance.json`,
  then loads each question through the P02 adapter.
- It scores each method with the P04 official retrieval scorer: turn recall and
  NDCG at 5 and 10, plus session recall at 5. It also scores evidence coverage
  per budget: whether every evidence round (user messages labeled
  `has_answer`) is packed.
- Abstention questions, and questions without user-side evidence labels, are
  excluded from retrieval metrics, as the official rules require.
- The pinned file lists 211 of its 500 histories out of date order. The tool
  stable-sorts each history by date before loading and records the count under
  `adaptations`. Questions the adapter still rejects are listed under
  `excluded` with the reason, and count as missing in the official aggregate.
- Dense retrieval uses any OpenAI-compatible `/v1/embeddings` endpoint on
  loopback. Requests are batched by count (32) and by estimated size (6000
  tokens), because the server's context is shared by a batch. A text over 16000
  bytes is cut at a character boundary and counted. Vectors are cached on disk
  under the SHA-256 of their text; keep a separate cache folder per model and
  configuration.

## Budgeted answers in ltm-compare

The `bm25_budget` condition packs BM25-ranked user rounds into
`context_budget_tokens` and gives them to the reader chronologically. The budget
and retriever are recorded in the manifest's `method_parameters`. It is not
available for dense or hybrid yet, because `ltm-compare` has no embeddings client.

`scripts/run-qwen-budget.ps1` runs the fresh correction histories through local
Qwen. `scripts/report-qwen-budget.ps1` summarizes them with the same
stated-grade reading as `scripts/report-qwen-replay.ps1`.
