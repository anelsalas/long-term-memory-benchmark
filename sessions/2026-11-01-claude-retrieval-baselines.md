# Claude retrieval baselines and LongMemEval answer runs

November 1, 2026. Claude (Opus 5.5). Summary, not a transcript.

Branch `claude/retrieval-baselines`, test-first C++17, coordinated with Codex
through `C:\Users\anels\Source\ltm-coordination\` (append-only mailbox).
86/86 CTest on Windows ARM64 with VS 2026. Codex supplied the pinned dataset,
the 60-ID development list, the embedding server, and fixes to P02 (session
order, repeated IDs, include_all policy) and P04 (integer gold, repeated-ID
parity).

What was added:
- `retrieval` library: rounds, BM25, cosine, RRF, budget packing,
  `rank_rounds`.
- Embeddings: request/response parsing, a disk cache, a cache-only embedder,
  and a loopback Beast client that splits refused batches.
- `ltm-retrieval-eval`: official P04 metrics plus evidence-per-budget,
  `--history-policy`, `--singletons`.
- `ltm-compare`:
  - `bm25_budget`, `dense_budget` and `hybrid_budget` (embedding cache),
    `context_budget_tokens`;
  - `include_fresh: false`, `known_evidence` for public cases;
  - distractors with any local profile.
- Scripts for the benchmark Qwen server and the answer runs, plus
  dialogue-free reports.

Results (`reports/2026-11-01-retrieval-baselines.md`):
- **Retrieval, include_all, 46 scorable:** turn recall_all@10 is 91.3% dense,
  84.8% hybrid, 73.9% BM25, 2.2% recency.
- **Fresh Qwen, 512-byte Dumas profile:** BM25 at 50 tokens 20/20, keyword
  top-1 0/20.
- **LongMemEval answers, 54 strict, local Qwen3-0.6B as reader and judge:**
  question only 13.0%, BM25 37.0%, dense 42.6%, hybrid 44.4%, labelled
  evidence 66.7%. The reader is the bottleneck.

Machine findings:
- Visible RAM is 18.9 GB. The RTX Spark N1X GPU (12 GB of its own) is unused:
  no CUDA toolkit or Vulkan SDK is installed.
- Claude Code stops background jobs under memory pressure; long runs went
  detached with the user's approval.
- The MSVC llama.cpp build read prompts about 3× slower than a clang build of
  the same revision (the flags also differ).
- Greedy Qwen3 thinking loops in 7 of 270 cases.

No services left running: the benchmark Qwen server was stopped, and Codex
stopped the embedding server. Runtime artifacts (answers, caches,
superseded attempts) are in ignored `runtime/`.

Next:
- Try Qwen's recommended sampling instead of greedy.
- Try a stronger reader, ideally on the GPU (needs a toolkit install the user
  must approve).
- Count tokens with the server tokenizer to use the full 32k context.
- Add the official GPT-4o judge when the budget allows.

Root docs are left to Codex.