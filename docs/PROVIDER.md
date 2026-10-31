# Bounded provider lifecycle

P03 adds `Provider`, `ProviderRunner` and standard C++17 request/outcome types in
`long_term_memory_benchmark/provider.hpp`. Link `LongTermMemoryBenchmark::provider`.
An injected provider implements synchronous `generate(request, control)`.

The lifecycle validates the request, reserves its budget, dispatches one attempt,
validates the reply, settles usage and classifies the outcome. Reservation,
settlement and reply normalization are separate implementation operations.

## Dispatch and identity

Requests carry protocol version 1, case ID, fixed model identity, exact prompt,
input-token upper bound, output-token cap, response-byte cap and local dispatch ID.
The selected reader or judge comes from the experiment manifest. Each attempt
retains the request, prompt/response text checksums, provider request ID and reply.
CaseRecord captures the final answer/status/request ID; grades remain unset.

Outcomes retain the P01 manifest hash and a lifecycle hash covering that manifest
plus every policy field. Dispatch IDs additionally bind case, prompt, role, token
bounds and ordinal attempt. Record the transport implementation revision in the
manifest run parameters. Preserve runtime prompts/outcomes in approved storage.

## Integer budgets

`ProviderPolicy` declares total requests, tokens and cost; per-input/output-token
prices; fixed per-request price; byte caps; and maximum attempts. Prices and costs
use integer nanodollars (1 USD = 1,000,000,000 nanodollars).

Before dispatch, reserve the input upper bound plus output cap and their worst-case
cost. Checked arithmetic rejects overflow. A zero request/token cap admits zero
billable requests. Zero prices explicitly represent a declared free/stub service.
Token bounds and prices must include all billable activity supported by the adapter.

Reported input/output usage settles each reservation under those declared rates.
Unknown usage retains the full reservation. A safe-transient classification
guarantees the request was not accepted; absent usage in that classification
also guarantees no billing. Reported failure usage is charged. Request attempts
always consume the request counter.

Reported usage beyond its reservation is recorded, marks `usage_overrun` and stops
dispatch. Representable totals retain the reported overage. An unrepresentable
charge marks `accounting_overflow`; counters retain the reserved amount and raw
reported usage remains in the attempt for reconciliation. In that state counters
are incomplete accounting, and the runner is stopped.

The ledger is local to one runner instance. Its mutex serializes callers through
dispatch and settlement. Observe snapshots between calls; provider callbacks use
the supplied request/control rather than reentering runner accessors. Durable
ledger restoration and interruption/resume integration belong to P07.

## Outcomes and retries

| Provider outcome | Runner action |
| --- | --- |
| Success | Complete ungraded answer; settle known usage |
| Explicit safe transient | Retry within remaining attempts and budgets |
| Authentication/configuration/billing error | Failed case; stop the runner |
| Ambiguous submission or transport exception | Uncertain case; stop for reconciliation |
| Cancellation after dispatch | Uncertain case; retain conservative accounting |
| Invalid/oversized UTF-8 result or usage overrun | Uncertain case; stop for reconciliation |

The adapter maps a transient response to safe-transient only when nonacceptance
is established. This state machine performs bounded retries; adapter-specific
Retry-After/backoff belongs to the selected transport and uses the same control.
Error strings supplied by adapters must be sanitized for credential-safe artifacts.

## Cancellation and deadlines

`RequestControl` supplies a cancellation callback, monotonic clock and optional
deadline. The runner checks before each dispatch and passes the control to the
provider for cooperative polling. The network adapter enforces transport timeouts
and streaming body caps. Synchronous C++ calls return when their provider returns;
an adapter that ignores cancellation cannot be forcibly interrupted here.

A successful response arriving after the deadline remains an observed completed
answer and has `completed_after_deadline` set. Failure to obtain an unambiguous
response requires reconciliation rather than a duplicate call.

## Acceptance

`ltm_provider_tests` runs deterministic injected-provider cases with no arguments.
CTest name: `ltm.unit.provider`. G01 verifies live endpoint access, credentials,
data-use terms, actual pricing/token bounds and the pilot budget before connecting
a network transport. Current observed acceptance covers the stubbed lifecycle.
