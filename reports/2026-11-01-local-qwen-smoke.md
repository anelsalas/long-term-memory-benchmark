# First measured local Qwen comparison

November1,2026. Coordinator observed actual model responses through the native
comparison command. Two local requests completed; APIspendingUSD0.

| Condition | Evidence messages | Answer | Expected | Correct | Reader walltime |
| --- | --- | --- | --- | --- | --- |
| Full history | 3 | 82 | 82 | Yes | 5324.2852ms |
| Retrieved lexical top1 | 1 | 23 | 82 | No | 941.6513ms |

The full history contains Lina's original23, Robin's63 and Lina's correction82.
Lexical retrieval selected the original grade and omitted the correction. This
is an observed reference-retrieval failure. Both outputs were graded from the
independent ledger. Exit0 means both cases completed and were graded; it does
not imply both answers were correct. One question in two conditions supplies a
plumbing smoke result; a larger frozen evaluation supplies research estimates.

The existing Qwen3-0.6B-Q8_0 service limits context to512 serialized bytes and
output to32tokens. The explicit compact_local_smoke profile uses dated evidence
and an integer question, zero distractors, seed42 andtop_k1. External HTTPS
benchmark defaults remain separate. See the exampleconfig and COMPARE.md.

The service reported2 completiontokens per answer. Inputtokens are unavailable;
1088 accountedtokens are conservative retained reservations, not measured usage.
CPU/RAM snapshots cover the runner process. Modelworker resources are outside
that collector. Initial health was cold/restartable; cold/warm state differs
between arms, so the timings do not establish a retrieval speed improvement.
The server's finish_reason is generic; final boundary remains an API limitation.

Reviewed machine-independent measurements are in the companion JSON. Full prompts,
responses, modelmetadata, resource samples and sourceIDs remain in ignored runtime
artifacts. Source revision includes base00c97e0 and a digest of the local adapter
working sources. Existing audio/model service settings were preserved.
