# Existing Qwen endpoint discovery

Terms are explained in the [glossary](../docs/GLOSSARY.md).

November 1, 2026, approximately 10:36 America/New_York. Read-only discovery;
generation requests made: **0**. Existing audio and model services remained running.

## Observed connection

- `http://127.0.0.1:8766/v1/chat/completions`, model `qwen3-0.6b-q8_0`.
- `GET /health`: HTTP200, backend `approved_qwen_worker`, `accepting:true`,
  `active:false`, `model_loaded:false`, status `restartable`, last error `none`.
  This establishes an available cold-start route, rather than a loaded model.
- `GET /v1/models`: HTTP200; profile `qwen-local`, context512 bytes,
  maximum8 messages, maximum32 output tokens, reply4096 bytes.
- Listening process26240 is `dumas_resident_service.exe`, from the live Dumas
  checkout's `build-resident-model-service-thinking/Release` directory.
- Port8098 is PocketBase; `/health` and `/v1/models` both returned404.
  Port8768 returned403 for these probes; its protected plan service is unrelated.

## One bounded request

```http
POST /v1/chat/completions HTTP/1.1
Host: 127.0.0.1:8766
Content-Type: application/json

{"model":"qwen3-0.6b-q8_0","messages":[{"role":"user","content":"Lina's astronomy score was 63. Correction: it is 81. What is the corrected score? Reply with the integer only."}],"stream":false,"max_tokens":8}
```

This is an illustrative compact synthetic request, **not an executed result**.
The service cold-starts a restartable worker on chat. Request fields are exactly
`model`, `messages`, optional `stream`, optional `max_tokens`1..32. Message fields
are exactly `role` and nonempty string `content`; roles are system/user/assistant.
Preflight requires the sum of UTF-8 content bytes plus role bytes plus4 per
message to fit512. Body limit32KiB; response text limit4096 bytes.
Send no Origin header. Local processes require no API key. Sampling fields,
including `temperature`, are rejected rather than applied.

Nonstream success contains `choices[0].message.content` and
`usage.completion_tokens`. Prompt and total token counts are unavailable.
`finish_reason` is a generic `stop`; the worker's actual end/length reason is
not forwarded. Record observed output tokens and unavailable input usage
separately. HTTP429 means busy,400 invalid request,502 worker failure,504 deadline.
The default request deadline is30 seconds and includes readiness waiting.

## Model and implementation evidence

Inspected the live checkout's `tools/resident-model-service/request.cpp`,
`main.cpp`, README, worker `llama_backend.cpp`, Windows `process_port.cpp`,
the active build's CMake cache and generated `windows-port/approved_profile.hpp`.
The generated profile selects `build-local-model-worker-retention/Release/`
`dumas_model_worker.exe` and the existing `Qwen3-0.6B-Q8_0.gguf` artifact under
the private Dumas worktree's `build-deps/llama-smoke/models/` directory.
The model exists,639446688 bytes. The source requires model SHA-256
`9465e63a22add5354d9bb4b99e90117043c7124007664907259bd16d043bb031`
at launch; this review did not recompute the model hash or certify binary/source
identity. Generated worker hash:
`58bef0aefafdc5f29173bfefd17f343ef4b3b20dc04a41aa61d86deedda46973`.

The inspected worker uses1024 context tokens, two CPU threads, a fresh sampler
per request, top-k20, top-p0.8, temperature0.7 and seed42; thinking is disabled.
These are source-profile settings, not request-configurable parameters.

## Benchmark adaptation

The current verbose benchmark JSON prompts need a byte-size check against this
service. A compact synthetic paired experiment can fit the existing512-byte
profile and provide actual answers, correctness and wall-clock timing now.
Declare its compact rendering and restricted context as adaptations. Preserve
source mappings and evaluator gold in separate artifacts. Keep full-history and
retrieved prompts within the same service bounds and output limit.

The benchmark transport needs an explicit loopback HTTP mode, its restricted
request schema, and completion-only usage support. Measure local elapsed time;
report prompt/total usage as unavailable. Public LongMemEval reproduction needs
a separately reviewed larger-context inference route. This discovery provides
endpoint readiness and source-contract evidence; live generation acceptance is
the next check owned by the coordinator.

The initial sandbox probes were socket-denied; authorized elevated read-only
probes produced the HTTP observations above. No service restart, setting change,
model download, credential access or paid request occurred.
