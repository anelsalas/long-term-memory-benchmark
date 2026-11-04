# Glossary

Plain-language meanings of the terms used across this repository's README,
docs and reports.

## The benchmark

- **LongMemEval.** A public benchmark for long-term chat memory. Each question
  comes with a long history of earlier chat sessions, and the answer is hidden
  somewhere in that history.
  - **LongMemEval_S** is the version used here: about 50 sessions, or roughly
    115k tokens, per question.
  - **`_cleaned`** names the dataset revision with known errors fixed.
- **LoCoMo.** Another public long-conversation memory benchmark, used in the
  Mem0 paper. Not run here yet.
- **Session.** One earlier conversation in a question's history.
- **Round.** One user message plus the assistant replies that follow it. The
  retrieval methods here pick whole rounds.
- **Turn.** A single message.
- **Question types.**
  - **Single-session-user** and **single-session-assistant:** the answer is in
    one session, said by the user or by the assistant.
  - **Single-session-preference:** the reply should use a preference the user
    stated earlier.
  - **Multi-session:** the answer needs facts from two or more sessions, for
    example adding up things mentioned on different days.
  - **Knowledge-update:** a fact changed over time; the newest value is right.
  - **Temporal-reasoning:** the answer needs dates or the order of events.
- **Abstention question.** The question ID ends in `_abs`. The history does not
  contain the answer, and the correct reply is to say so.
- **Development questions.** The 54 questions used for these experiments,
  chosen to cover every question type. They are not the full benchmark.
- **Strict policy.** A question is dropped if any of its history is dated after
  the question was asked, since a real user could not have said it yet. That
  removed 6 of the 60 selected questions.
- **Labelled evidence.** The benchmark marks which sessions and messages contain
  the answer.
  - **Labelled evidence (control)** gives the reader only those messages. It
    shows how a reader does with perfect retrieval, the upper bound for any
    memory method.
  - **Evidence present** counts questions where at least one labelled message
    made it into the reader's context.

## Who answers and who grades

- **Reader.** The model that reads the supplied context and answers the
  question.
- **Judge.** A model that compares the reader's answer with the benchmark's
  reference answer and says correct or not. It uses the official LongMemEval
  judge prompts, which expect a yes or no.
- **Judge agreement.** How often two different judges give the same verdict on
  the same answer.
- **Effort (low, medium, high).** How much the model reasons before answering.
  Higher effort is slower and uses more of the plan.
- **Qwen3-0.6B.** A small open model, 0.6 billion parameters, run on this
  computer's CPU. **Qwen3-Embedding-0.6B** is its sibling that makes embeddings
  (see Dense search).
- **GPT-6.1-sol** and **Claude Opus 5.5.** Large hosted models, reached through
  the owner's signed-in Codex and Claude Code subscriptions.

## Ways of choosing what the reader sees

The reader is never given the whole history in these runs. Each method picks
part of it.

- **Context budget.** The most text a method may hand the reader: here about
  6,000 tokens.
- **Token.** The unit models count text in, roughly three quarters of an English
  word. Here tokens are estimated as UTF-8 bytes divided by 4.
- **Question only** (`no_memory`). No history at all; shows what the reader can
  guess.
- **Newest first** (`recency`). The latest rounds that fit the budget.
- **BM25.** Classic keyword search, as used by search engines for decades. It
  scores each round by how many of the question's words it shares. Rare words
  count more than common ones, and very long rounds get less credit per match.
  It knows nothing about meaning: "car" does not match "automobile".
- **Embedding.** A list of numbers, a vector, that a model computes from a piece
  of text so that texts with similar meaning get similar vectors.
- **Dense search** (dense vector search, dense embeddings, `dense_budget`).
  Every round and the question are turned into embeddings, and the rounds whose
  vectors are closest to the question's are picked. "Dense" refers to these
  vectors, in which every number carries some meaning. BM25's word counts are
  called "sparse". Dense search matches meaning, so "car" can match
  "automobile".
- **Hybrid** (BM25 + dense, RRF). Merges the BM25 and dense rankings into one.
- **RRF (reciprocal rank fusion).** The merging rule hybrid uses. Each round
  gets 1 / (60 + its rank) from each list, and the scores are added. Rounds near
  the top of either list rise.
- **Packing.** Taking ranked rounds in order until the budget is full, then
  showing them to the reader in date order.
- **Mem0.** An open-source memory system. Before any question is asked, it reads
  each session with a language model and writes short facts ("memories"), for
  example "User moved to Denver in March 2023". At question time it searches
  those memories instead of the raw messages.
- **Ingestion.** Mem0's up-front step of reading every session and writing
  memories. It costs one model call per session, once per history.
- **Memory events.** Memories Mem0 added or changed during ingestion.
- **Graphiti.** Another memory system that stores facts as a graph of people,
  things and relations. Planned, not run yet.
- **RAG (retrieval-augmented generation).** The general pattern of retrieving
  text and giving it to a model to answer from. BM25, dense and hybrid are
  simple RAG.
- **Full history.** Giving the reader the whole ~115k-token history. Not run yet
  because of its cost.

## Measuring

- **Accuracy** (correct out of N). The share of answers the judge accepted.
- **Retrieval at top 5 / top 10.** Share of questions where every labelled
  evidence turn is among the first 5 (or 10) results of a method. This scores
  the search alone, without a reader.
- **Recall@k.** The share of the evidence that appears in the top k results.
- **NDCG@k.** Like recall, but gives more credit when evidence appears higher in
  the top k.
- **Sign test and p-value.** A check of whether wins could be luck. With 3 wins
  and 0 losses, a fair coin gives an all-one-side result 25% of the time
  (p = 0.25), too often to call the difference real. Values below 0.05 are the
  usual bar.
- **Latency, median, p95.** How long a step takes. The median is the middle run;
  p95 is the time 95% of runs beat, the slow tail.
- **Cold / warm.** Cold is the first run, with loading costs; warm is a later run
  with the model already loaded.
- **Run-to-run variance.** How much results change if the same run is repeated.
  Not measured here: each configuration ran once.

## Tools and code

- **Codex** and **Claude Code.** OpenAI's and Anthropic's command-line coding
  assistants, used here as subscription access to their models.
- **CLI.** A command-line program.
- **CLI framing.** Extra instructions a CLI adds to every model call: about
  5,000 tokens for Codex, about 700 for Claude Code. A direct API call would not
  include them.
- **`ltm-cli-bridge`.** This repo's local program that makes those CLIs look like
  a standard chat-completions server, so the benchmark can call them.
- **Loopback** (127.0.0.1). A network address reachable only from the same
  computer.
- **GGUF, Q8_0, quantization.** GGUF is a model file format for local inference.
  Q8_0 stores each weight in 8 bits instead of 16, halving the size with little
  quality loss.
- **llama.cpp.** The open-source program that runs GGUF models locally.
- **C++17, Boost, Boost.Beast.** The language version and libraries the
  benchmark is written in; Beast provides HTTP.
- **CMake, CTest.** The build system and its test runner.
- **ARM64.** The processor type of this Windows Surface computer.
- **TDD (test-driven development).** Write a failing test first, then the code
  that makes it pass.
- **Ports and adapters.** A design where the core logic talks to interfaces
  ("ports"). Real or fake implementations ("adapters") plug in, so tests run
  without models or networks.
- **Manifest.** The JSON record of a run's exact settings, versions and hashes,
  so the run can be checked and repeated.
- **SHA-256.** A fingerprint of a file or text; any change gives a different
  value.
