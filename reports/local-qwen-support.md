# Local Qwen adapter support

November1,2026. Native runner worker summary and handoff.

Implemented explicit numeric-loopback execution in http_provider.hpp/.cpp and
compare_main.cpp. Default external HTTPS behavior remains governed by its TLS,
credential and positive cost gates. Local configuration requires zero prices,
zero spending allowance, and positive request/token caps. No environment
credential lookup occurs in local mode. Both profiles preserve exact model
identity checks, bounded responses/timeouts, disabled redirects and conservative
uncertain-submission handling.

Local requests bypass proxies and permit empty bearer credentials. The minimal
local payload omits `n`; optional temperature is omitted when null. Completion-
only local usage remains reported metadata without fabricated input tokens.

Behavioral TDD: added a local credential-free test to the unchanged implementation;
observed failure `A valid HTTPS endpoint is required`. Implemented explicit scope
and expanded checks for IPv6 loopback, DNS/lookalike/nonloopback rejection,
external credential enforcement, minimal payload and partial usage. Native ARM64
Release builds of ltm_http_provider_tests and ltm-compare succeeded; all7 HTTP
behavior groups passed. Local fresh dry-run produced2 planned requests and zero
dispatch with no key configured. No inference calls or paid calls were made.

The existing server identified by the research worker uses model
qwen3-0.6b-q8_0 and512 serialized context bytes with32 output tokens. Implemented
local-only compact_local_smoke with fresh_distractors0: dated source text and
question retain evidence while removing prompt JSON overhead. Exact512-byte
role-framing preflight occurs before dispatch. The manifest records profile,
distractor count and unconditional service finish-reason compatibility limit.
Generic benchmark defaults remain intact. Both judge/reader caps must fit32.

Observed final native Release build and scoped CTest2/2 pass (HTTP provider and
CLI integration); local dry-run saved2 requests with zero dispatch and null
model accuracy. Initial dry-run failed the declared judge-cap check because its
inherited512 judge limit exceeded reader32; setting both32 resolved it. Root
was notified that the executable is ready for the authorized local comparison.
Coordinator owns that actual inference, measured result and publication.
Owned files: http_provider.hpp, http_provider.cpp,
http_provider_tests.cpp, compare_main.cpp, docs/COMPARE.md and this report.
