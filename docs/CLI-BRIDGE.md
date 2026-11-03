# Subscription CLI bridge

`ltm-cli-bridge` lets `ltm-compare` use Claude and GPT models through the owner's
own signed-in Claude Code and Codex CLIs. It serves OpenAI-shaped chat
completions on loopback, so `ltm-compare` reaches it in its local execution mode
with zero prices. Each request runs one CLI process:

| CLI | Command | What is turned off |
| --- | --- | --- |
| Claude Code | `claude -p --model M --effort E --output-format json` | Tools (`--tools ""`), settings files, MCP servers, slash commands, saved sessions. `--system-prompt` replaces the default prompt. |
| Codex | `codex exec -m M -c model_reasoning_effort=E --json -` | User config, saved sessions, web search and the tool features (shell, exec, code mode, browsers, apps, plugins, agents, images). `model_instructions_file` replaces the base instructions. |

**Use and limits**
- **Personal use only.** It's for the owner's benchmark runs on their own
  machine. Each CLI signs itself in, and the bridge never sees a token. Usage
  counts against the owner's plans. This follows the reasoning in Dumas's
  `tools/claude-code-bridge`.
- **Same instruction for both CLIs.** Each CLI gets the same neutral system
  prompt and the prompt on standard input. Each runs in an empty folder, so
  there is nothing nearby to read.
- **Codex tool use.** Any Codex tool-use event invalidates the answer and the
  request fails with `tool_use`.

## Settings

```json
{
  "claude_executable": "C:/Users/me/.local/bin/claude.exe",
  "codex_executable": "C:/.../codex-win32-arm64/vendor/aarch64-pc-windows-msvc/bin/codex.exe",
  "system_prompt": "You are a helpful assistant.",
  "codex_instructions_file": "C:/.../runtime/cli-bridge/instructions.txt",
  "working_directory": "C:/.../runtime/cli-bridge/empty",
  "timeout_ms": 540000, "retries": 2, "retry_delay_ms": 10000,
  "profiles": [
    {"id": "claude-opus-5-5/high", "cli": "claude", "model": "claude-opus-5-5", "effort": "high"},
    {"id": "gpt-6.1-sol/high", "cli": "codex", "model": "gpt-6.1-sol", "effort": "high"}]
}
```

- **Profile IDs.** A profile's `id` is the model name that clients send and get
  back, so it names both the model and the effort.
- **Served model check.** Claude Code reports the model that answered; a
  different model is refused (`model_mismatch`). Codex does not report one.
- **Startup.** The bridge writes `system_prompt` to `codex_instructions_file`,
  and refuses to start unless `working_directory` is empty.
- **Run it:** `ltm-cli-bridge --settings FILE --port 8773 --failures DIR`.
  Run a copy outside the build folder, so rebuilding doesn't collide with the
  running executable.

## Behavior

- **Concurrency.** `max_concurrent` limits simultaneously running CLI attempts.
  Additional requests wait for a slot; a missing or zero value leaves this
  limit unrestricted. Each retry acquires a slot separately.
- **Call budget.** `max_calls` optionally caps actual CLI attempts across the
  process, including retries; exhausted requests receive HTTP 429. A missing
  or zero value leaves this cap unrestricted. Set a positive limit for benchmark
  runs. Restarting the bridge starts a new counter, so preserve run accounting.

- **Endpoints.** Only `POST /v1/chat/completions` (one user message, no
  streaming) and `GET /health`, on 127.0.0.1. Requests that carry an `Origin`
  header, or a `Host` other than the bridge's own loopback port, get 403. This
  blocks ordinary cross-origin browser requests. Other local processes can
  still submit requests while the bridge runs; it has no client authentication
  or server-side spending cap.
- **Concurrency.** Each connection gets its own thread, so parallel workers
  don't queue behind a slow reply.
- **Child processes.** Each runs in a kill-on-close Job Object, stopped at
  `timeout_ms` (504, not retried). Standard error goes to NUL.
- **Failures.** A run that ends without an answer returns 502 with a short code:
  `cli_exit_N`, `cli_error` (a failed turn), `no_answer`, `tool_use` or
  `unreadable_output`. It is retried `retries` times, waiting `retry_delay_ms`
  multiplied by the attempt number. Retries consume plan usage and may use paid
  credits depending on the account.
- **Failure log.** With `--failures DIR`, each failed run's code and output go
  to their own file. Keep that folder in ignored runtime storage.
- **Codex transport notices.** Codex prints top-level `error` events while it
  reconnects, for example after a WebSocket 403 that ends in a fallback to HTTPS.
  These aren't failures: only `turn.failed`, or a turn without an answer, is.

## Adaptations to record with results

- **Temperature.** Neither CLI accepts one. The judge's temperature 0 is sent
  but not applied.
- **Output caps.** The CLIs don't enforce output caps. `ltm-compare` still
  checks reported usage against its reservation.
- **Token counts.** Input tokens include each CLI's own framing:
  - Codex adds about 5,000 tokens of base instructions and tool schemas, even
    with tools disabled.
  - Claude Code adds about 700 tokens.
  - Declare a `chat_framing_token_allowance` that covers this.
- **Finish reason.** `finish_reason` is always `stop`; it is not evidence that
  generation ended naturally.

Code: `include/long_term_memory_benchmark/cli_bridge.hpp`, `cli_bridge_host.hpp`
and their sources. Tests: `ctest -L cli_bridge`. The host tests use `sort.exe`,
`cmd.exe` and `PING.EXE` and an in-process HTTP client; no test launches a
model CLI.

## Re-judging saved answers

`ltm-rejudge` grades frozen answers again with another judge.

```powershell
scripts/collect-judge-prompts.ps1 -RunDirectory runtime/qwen-longmemeval -Output items.json
build-vs2026/Release/ltm-rejudge.exe --items items.json --endpoint http://127.0.0.1:8773/v1/chat/completions `
  --judge gpt-6.1-sol/high --out verdicts.json --workers 2
```

- **Input.** It sends each saved official judge prompt unchanged and reads the
  reply with P04's strict yes/no parser. The prompt already holds the question,
  the gold answer and the frozen answer, so only the judge differs.
- **Resume.** Rerunning with the same `--out` retries only failed items.
- **Reporting.** `scripts/report-reader-comparison.ps1` combines several readers'
  runs with their verdicts and a second judge's, for agreement.
