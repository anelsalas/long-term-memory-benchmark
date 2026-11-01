# Model access and pilot choice

Checked October 31, 2026 against official provider documentation. Account access
and effective project quotas are verified when configuring G01.

## Current local-first execution

November1: the user chose the running Qwen3-0.6B service for the first model run.
Two actual requests completed with zero APIspending; the measured report records
one correct full-history answer and one incorrect retrieved answer. The compact
local smoke uses existing512-byte/32-output-token service bounds. The APIshortlist
below remains optional for later historical/current-model comparisons.

## Two evaluation tracks

Historical reproduction pins the exact reader/judge snapshot specified by the
selected paper setup. The official OpenAI schedule lists GPT-4o August/November
2024 and GPT-4o-mini July 2024 shutdown for January 14, 2027. Their current
model pages mark them deprecated. A dated account-access check is required.
Sources: [deprecations](https://developers.openai.com/api/docs/deprecations),
[GPT-4o](https://developers.openai.com/api/docs/models/gpt-4o),
[GPT-4o mini](https://developers.openai.com/api/docs/models/gpt-4o-mini).

The engineering pilot uses an accessible current model held fixed across
conditions. Its result is an adapted comparison. A new model changes the
historical comparison; record that substitution in the manifest.

## Initial shortlist

| Purpose | Candidate | Declared tradeoff |
| --- | --- | --- |
| Low-cost plumbing pilot | Gemini 3.5 Flash-Lite | Current economical candidate; validate answer quality and returned version |
| Stronger paired comparison | Gemini 3.8 Flash | Current Flash candidate; record thinking usage and sampling configuration |
| Historical reference | Exact GPT-4o / mini snapshots from selected experiment | Time-limited service availability; match reader/judge roles to original setup |
| Additional current-provider check | GPT-6 Luna | Official current small-model option; adapter/settings and account acceptance follow |

Google lists 3.5 Flash-Lite at $0.30 input/$2.50 output per million tokens.
3.8 Flash is $0.75/$3.75 through December 31, 2026, with later announced prices.
Thinking is included in billed output. Recheck tariffs before execution.
Sources: [Google pricing](https://ai.google.dev/gemini-api/docs/pricing),
[model catalogue](https://ai.google.dev/gemini-api/docs/models),
[GPT-6 Luna](https://developers.openai.com/api/docs/models/gpt-6-luna).

## Transport and reproducibility

Google documents an OpenAI-compatible chat endpoint. Configure the full HTTPS
URL, exact model ID, requested version and credential environment-variable name.
Validate the provider's returned model identity and usage before widening runs.
Use an immutable snapshot when available; aliases require recorded returned
identity/time and a stated reproducibility limit.
[Compatibility reference](https://ai.google.dev/gemini-api/docs/openai).

The first transport uses a bounded Chat Completions subset. Each candidate needs
an acceptance check for output cap, temperature, reasoning settings, usage and
context limits. Compute input bounds conservatively; a byte upper bound can
reject long histories that fit under a tokenizer. Add a validated tokenizer to
recover that capacity before historical full-context runs. Declare omissions and
truncation before scoring.

Google documents default thinking settings for new Gemini models. The original
10-token grader cap needs a declared adaptation for a thinking model; sample
plans use a512-token output cap, subject to live acceptance. Record defaults and
returned reasoning usage. A completed HTTP response with a truncated answer is
an explicit failure/uncertain outcome.

## Proposed pilot envelope

One published case and one fresh case, each with full history and retrieved
history: four reader requests. Two official public-case judge requests bring the
planned total to six; fresh score answers have deterministic ledger grading.
Reserve a maximum of eight attempts, one million total accounted tokens and
USD1 total under conservative per-token tariffs. These are proposed settings;
G01 records the user-approved cap and actual accessible provider.

Dry-run saves planned evidence/prompts/bounds with zero dispatch. Execute mode
uses the same plan and configured caps. Save reader output before forming judge
requests. Report reader/judge usage separately and total usage together. Record
cache/reasoning costs and unknown usage explicitly. Application budget accounting
controls this runner; the provider account supplies the billing authority.

Google distinguishes free-tier content-use terms from paid-tier terms. Choose
terms for the actual project and record the policy used; use public/fictional
inputs for the initial pilot. Pricing-page metadata supplies this distinction.

## Next acceptance

Build the command; inspect dry-run artifacts; select an accessible endpoint and
protected credential; confirm cap; execute the fixed six-call plan; audit output,
usage and errors. Then freeze the configuration for a larger development subset.
