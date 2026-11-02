# Claude: stronger readers on the same LongMemEval evidence

November 2, 2026. Claude (Opus 5.5). Summary, not a transcript.

Branch `claude/stronger-readers`, from main `1f686c7`. Test-first C++17 with
Boost.Beast. 105/105 CTest on Windows ARM64 with VS 2026; 19 tests are new.

The user asked to run GPT and Claude on the same 54 questions and the same
retrieved evidence as the local Qwen run. They also asked for the judge at the
highest effort.

What was added:
- **`ltm-cli-bridge`.** Loopback chat completions served by the owner's
  signed-in `claude -p` and `codex exec`:
  - tools off and a shared neutral system prompt;
  - an empty working folder and kill-on-close job objects;
  - Origin/Host checks;
  - retries and a failure log.
  - See `docs/CLI-BRIDGE.md`.
- **`ltm-rejudge`.** Sends saved official judge prompts, unchanged, to another
  judge, using P04's strict parser. It resumes and retries only failures.
- **Scripts:**
  - `collect-judge-prompts.ps1`;
  - `report-reader-comparison.ps1`;
  - `run-qwen-longmemeval.ps1`, which gains `-Judge`, `-Shard`/`-Shards`,
    `-Temperature null` and the framing and answer-byte options. The script also
    stopped unrolling condition groups (a PowerShell `, $array` pitfall).

Results (`reports/2026-11-02-reader-comparison.md`), judged correct by
GPT-6.1-sol/high out of 54:

| Condition | Qwen3-0.6B | Claude Opus 5.5 | GPT-6.1-sol |
|---|---|---|---|
| Dense, 6k budget | 35.2% | 88.9% | 85.2% |
| Labelled evidence | 66.7% | 94.4% | 92.6% |

The remaining retrieval gap for strong readers is mostly multi-session
questions: 6/10 with dense retrieval against 10/10 with the labelled evidence.

Observed problems:
- **Codex reconnect events.** Codex prints top-level `error` events while it
  reconnects after a Cloudflare 403. The bridge first treated them as failures.
- **Claude usage limit.** The plan's limit was reached once, and the affected
  questions were rerun.
- **Incomplete runs.** These were set aside whole and rerun.

Nothing is left running. The bridge was stopped after the last judge run.
