# Handoff: stronger readers and re-judging

November 2, 2026, from Claude. Branch `claude/stronger-readers`.

## State

- **Readers.** Claude Opus 5.5 and GPT-6.1-sol answered all 54 strict
  LongMemEval_S development questions in five conditions: 270 answers each,
  all judged.
- **Judging.** All three readers, Qwen included, are judged by GPT-6.1-sol/high,
  with Claude Opus 5.5/high as a second judge.
- **Report.** The report and its dialogue-free record are in `reports/`.
- **Runtime storage (ignored).** Raw answers, judge replies, superseded
  incomplete runs and the bridge failure log stay in `runtime/`.

## To reproduce

1. Build, then copy `ltm-cli-bridge.exe` out of the build folder and start it
   with a settings file (see `docs/CLI-BRIDGE.md`). Profiles used:
   `claude-opus-5-5/high`, `gpt-6.1-sol/high`; `retries` 2, `retry_delay_ms`
   10000, `timeout_ms` 540000.
2. Start the run workers, one per shard:
   `scripts/run-qwen-longmemeval.ps1 -Model <profile> -Judge gpt-6.1-sol/high`
   with these options:
   - `-Endpoint http://127.0.0.1:8773/v1/chat/completions`
   - `-ContextTokens 1000000 -OutputTokens 64000 -JudgeOutputTokens 16000`
   - `-FramingTokens 16000 -Temperature null -AnswerBytes 65536`
   - `-Shard i -Shards 3 -Conditions no_memory,bm25_budget,dense_budget,hybrid_budget,known_evidence`
3. Move any run with `complete: false` aside and rerun the script; it resumes.
4. Re-judge with `scripts/collect-judge-prompts.ps1` and
   `ltm-rejudge --judge claude-opus-5-5/high`.
5. Report with `scripts/report-reader-comparison.ps1`.

## Next

- **Mem0 and Graphiti, against dense retrieval.** Compare them on these same
  54 questions, with a strong reader, at a matched budget. Multi-session
  questions are where retrieval loses the most (6/10 against 10/10 with the
  labelled evidence).
- **Full history (~115k tokens) with the strong readers.** It fits their
  contexts, at about 6M input tokens per reader.
- **An official GPT-4o judge.** Still needs the G01 budget decision.
