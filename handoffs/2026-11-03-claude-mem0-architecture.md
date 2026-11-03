# Mem0 on LongMemEval: architecture for Codex to implement

November 3, 2026, from Claude (architecture only). **Codex writes the code.**
The user's constraint: **no Claude model calls**, and Claude's own work is kept
to design and one review.

## Goal

Measure Mem0 against the dense-retrieval baseline. Everything else stays fixed:
the questions, the reader, the context budget and the judge. Then reuse the same
port for Graphiti.

## Fixed from the reader comparison

These come from `reports/2026-11-02-reader-comparison.md`.

- **Reader: `gpt-6.1-sol/high`** through `ltm-cli-bridge` (Codex plan).
- **Judge: `gpt-6.1-sol/high`.** Official prompts and the P04 parser.
- **Budget.** 6,000 estimated tokens (UTF-8 bytes / 4) of memory text.
- **Baselines.** The existing `gpt-longmemeval` results serve as baselines and
  are not rerun:
  - `dense_budget`: 46/54, and 6/10 on multi-session questions;
  - `known_evidence`: 50/54, and 10/10 on multi-session questions.
- **Bridge settings for this work list only GPT profiles.** With no Claude
  profile, nothing can reach Claude.

## Phases

1. **Measure one question.** Ingest one multi-session question. Record LLM
   calls, tokens, wall time and memory count. Stop and report if a projection to
   10 questions exceeds the Phase 2 cap.
2. **The 10 multi-session questions**, where retrieval loses the most. The
   target table, GPT reader:

   | Context | Correct |
   |---|---|
   | Dense | 6/10 (existing) |
   | Mem0 | x/10 |
   | Labelled evidence | 10/10 (existing) |

   - **Cap:** 1,500 bridge calls in total.
3. **All 54 questions**, only after the user reviews Phase 2.

## Components

### 1. Mem0 sidecar: `adapters/mem0/` (Node, glue)

- **Package.** Upstream `mem0ai` OSS 3.3.1, pinned with a committed lockfile and
  installed once (no installs during runs). Keep upstream attribution. This is
  the public counterpart of Codex's private Dumas pilot adapter.
- **Endpoint.** Loopback HTTP only. Refuse requests with an Origin header or a
  foreign Host, as `ltm-cli-bridge` does.
- **Configuration.**
  - **LLM.** OpenAI-compatible, `baseURL http://127.0.0.1:8773/v1`. Use the
    smallest fast GPT in the account catalog that returns valid Mem0 JSON on the
    Phase 1 question. Mem0's paper used a mini model. Otherwise use
    `gpt-6.1-sol` at low effort. Record the choice.
  - **Embedder.** The local Qwen3-Embedding-0.6B server on 127.0.0.1:8771, the
    same model as the dense baseline.
  - **Vector store.** SQLite on disk:
    `runtime/mem0/<config-sha>/<question_id>.sqlite`.
  - **Telemetry and history** off.
  - **Prompts.** Keep Mem0's upstream extraction and update prompts unchanged.
    Record any adaptation and its reason.
- **API.**
  - `POST /ingest {namespace, sessions:[{session_id, date, rounds:[{round_id, user, replies[]}]}]}`
    - Calls Mem0 `add()` once per session, with that session's messages.
    - Each message passes through `metadata {session_id, session_date, round_ids}`.
    - The session date must reach the memory text or its timestamp. Verify
      which works, and record it.
    - Resumable per session and idempotent per namespace and config hash.
    - Returns `{memories, llm_calls, input_tokens, output_tokens, wall_ms}`.
  - `POST /search {namespace, query, limit}` returns, in rank order,
    `[{memory_id, text, score, created_at, metadata}]`.
  - `GET /health`.
  - `GET /config` returns versions, model IDs, prompt SHA-256s and the config
    SHA.

### 2. `ltm-cli-bridge` changes (C++, test-first, small)

- **System message.** Accept an optional leading system message, because Mem0
  sends a system message and a user message.
  - Codex: write it to a per-request instructions file and pass
    `model_instructions_file`.
  - Claude: pass it with `--system-prompt`.
- **`response_format`.** Ignored and recorded. Verify that Mem0 parses the
  replies; Mem0 strips code fences itself.
- **Concurrency and caps.** Add `max_concurrent` (a semaphore, to stay under
  plan rate limits) and a call counter. Add an optional `max_calls` hard cap that
  returns 429 once reached.

### 3. `memory_budget` condition in `ltm-compare` (C++, test-first)

- **Port and adapter.** A `MemoryService` port: `search(namespace, question,
  limit)` returns ranked memories. Its adapter is a loopback Beast HTTP client,
  following `embeddings_http`.
- **Packing.** Pack memories in rank order into `context_budget_tokens` with
  `estimated_tokens`. Present them by session date, then rank, each with its
  date. The answer instruction stays the same, and the memories take the place
  of the rounds.
- **Search only.** `ltm-compare` only searches, reads and judges. Ingestion runs
  beforehand (component 1, from a small script), and its cost is reported
  separately.
- **Manifest.** Records the memory service's `/config` and its hash.
- **Evidence in context.** Session level: a packed memory whose `session_ids`
  includes a labelled evidence session. Memories that Mem0 merged (UPDATE)
  report what their metadata still holds; otherwise the value is null. Report
  this as session-level, not turn-level.

### 4. Report

- **Tool.** `report-memory-comparison.ps1`, or extend
  `report-reader-comparison.ps1`.
- **Accuracy rows.** Dense, Mem0 and labelled evidence, with the GPT reader, and
  by question type.
- **Costs.** Per question: ingestion LLM calls, tokens and time. Also memories
  per question, search latency and evidence presence.
- **Record.** Dialogue-free, in `reports/`.

## Tests (offline, no model calls)

- **Sidecar contract.** With a fake LLM and a fake embedder: ingest, then
  search, then metadata round-trips, and Origin/Host refusal.
- **Bridge.** System-message handling for both CLIs, `max_concurrent` and
  `max_calls`.
- **`memory_budget`.** Packing, date presentation and evidence-session
  matching, plus an `ltm-compare` CLI check against a fake memory service.
- **Full suite.** All existing tests (105) still pass.

## Later: Graphiti

Same `MemoryService` port; its sidecar wraps Graphiti's search. Graphiti needs a
graph database (Kuzu or FalkorDB embedded) and an LLM for entity extraction.
Use the same caps and phases.
