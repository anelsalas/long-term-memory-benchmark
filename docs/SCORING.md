# Official LongMemEval scoring adapter

Link `LongTermMemoryBenchmark::scoring` and include
`long_term_memory_benchmark/scoring.hpp`. P04 implements retrieval mathematics
and isolated QA judge requests using the pinned [LongMemEval source](https://github.com/xiaowu0162/LongMemEval/tree/9e0b455f4ef0e2ab8f2e582289761153549043fc).

## Retrieval

`score_official_retrieval(loaded_case, ranking, k, level)` accepts a complete
permutation of opaque corpus IDs. Session mode ranks session IDs; turn and
turn-to-session modes rank user-message IDs. Complete rankings allow the original
unique-session effective-k procedure. Duplicate, unknown or truncated rankings
are explicit compatibility errors. Positive k and consistent evaluator mappings
are required.

The evaluator reconstructs upstream corpus labels internally: answer-marked
session IDs become `noans` for user messages lacking answer evidence. Session
labels use whether any user message has evidence. `correct_docs` follows the
original corpus-ID marker rule, rather than substituting `answer_session_ids`.
Answer-marked user sources require Boolean annotations. Candidate IDs remain opaque.

| Field | Original meaning |
| --- | --- |
| recall_any | At least one target source found: 0 or 1 |
| recall_all | Every target source found: 0 or 1 |
| ndcg | Original binary-relevance discounted gain divided by ideal gain |
| effective_k | Actual prefix length used for the selected level |

The original DCG convention gives ranks one and two equal weight: first relevance
plus subsequent relevance divided by `log2(rank)`. Preserve that convention.
Turn-to-session mode strips the final turn suffix, expands the ranked prefix until
k distinct rewritten sessions are present, and retains repeated session entries
in nDCG. Its exhausted effective-k value can be corpus size plus one.

Cases whose original ID contains `_abs` are excluded from the retrieval aggregate.
Cases with no user-side `has_answer=true` are excluded separately. QA abstention
remains evaluated. Original empty-target-set behavior is retained for otherwise
eligible cases: any recall 0, all recall 1 and nDCG 0.

`aggregate_official_retrieval(expected_ids, scores)` records expected, supplied,
missing, eligible and both exclusion counts. Means use eligible cases; an empty
eligible denominator yields null optionals. Unknown/duplicate cases or invalid
metrics are rejected. Missing cases remain visible alongside original metrics.

## QA judge requests

`make_official_grade_request(loaded_case, frozen_response, model)` creates a
separate evaluator request with the original task-specific template and gold.
It preserves temporal off-by-one rules, update handling, preference rubrics and
abstention instructions. The default model identity is the source's
`gpt-4o-2024-08-06`; actual endpoint availability is verified at live-run setup.
The prompt is a single user-role message with one sample, temperature 0 and a
requested output limit of 10 tokens.

The request records SHA-256 for both the frozen template and full instantiated
prompt. String-valued gold is used as text, and integer gold is rendered in decimal,
matching Python string formatting. Other JSON types remain explicit compatibility errors.
The candidate object remains unchanged and its serialization excludes evaluator gold.

`parse_judge_verdict` accepts only a whitespace-trimmed, case-insensitive `yes`
or `no`. Other outputs yield `JudgeVerdict::invalid` for explicit accounting.
This is a documented adaptation from upstream's substring check; malformed
grades remain ungraded rather than becoming incorrect or silently accepted.
Provider transport and model execution are separate stages.

## Validation and provenance

Eight native behavior groups cover rank/target distinctions, effective-k mapping,
exclusions and denominators, bad inputs, template identity and strict verdicts.
`ltm_scoring_tests --parity-report` emits 540 ranking/cutoff/annotation cases;
`--prompt-report` emits the twelve task/abstention prompt instances used in review.
The report records exact observed parity and remaining environment limits.

The adapted formulas and templates are MIT-licensed upstream material, copyright
2024 Di Wu. Their complete license notice is retained in `src/scoring.cpp`.
References: [retrieval utilities](https://github.com/xiaowu0162/LongMemEval/blob/9e0b455f4ef0e2ab8f2e582289761153549043fc/src/retrieval/eval_utils.py),
[corpus labels and exclusions](https://github.com/xiaowu0162/LongMemEval/blob/9e0b455f4ef0e2ab8f2e582289761153549043fc/src/retrieval/run_retrieval.py),
[QA templates](https://github.com/xiaowu0162/LongMemEval/blob/9e0b455f4ef0e2ab8f2e582289761153549043fc/src/evaluation/evaluate_qa.py).
