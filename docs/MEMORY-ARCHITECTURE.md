# Dumas memory architecture: requirements and tests

Terms are explained in the [glossary](GLOSSARY.md).

## Objective

Keep the authoritative archive of conversations, images and other sources in the user's chosen cloud. A small local catalog and working context help Dumas find useful information quickly. Each cloud provider sits behind a replaceable storage interface. Cloud access, account selection, data transfer and spending follow the user's settings.

## Human memory as a design guide

These categories are engineering analogies. Tests establish Dumas's behavior; they do not prove biological equivalence.

| Human concept | Dumas category | Required behavior |
| --- | --- | --- |
| Working memory | Active context | Assemble recent chat, relevant instructions and selected evidence within a declared budget. |
| Episodic memory | Event | Record a dated experience and link it to original evidence. |
| Semantic memory | Fact / profile | Associate facts and preferences with people, topics, dates and sources. |
| Procedural memory | Procedure | Recall a user-approved routine or instruction with its scope. |
| Prospective memory | Task / reminder | Store an intention, due time and completion state. |
| Sensory memory | Current input | Handle live input as part of the active session; retain media only under the user's chosen policy. |

## Testable requirements

| ID | Requirement | Fast automated check |
| --- | --- | --- |
| M01 | Separate memories by person, category and topic. | Index fictional school and medical records for different people; query each and verify the references. |
| M02 | Preserve source IDs and dates; corrections retain earlier evidence. | Record a grade and a later correction; verify both the current and dated answer against their sources. |
| M03 | Search supports known tags and semantic cues; absent evidence remains absent. | Search by tag and reworded question, then query an unrecorded fact. Measure evidence recall and abstention. |
| M04 | Keep the local catalog bounded and distinguish it from the authoritative archive. | Fill a catalog to capacity, verify failure preserves existing entries, and verify an incomplete catalog triggers an archive search. |
| M05 | Changes or deletions do not leave stale local entries. | Update and delete records, then check local references and remote results. |
| M06 | A provider adapter stores and retrieves through the same contract, with explicit identity and access rules. | Run the same fictional fixtures against the fake and each configured real backend; verify access denial and error handling. |
| M07 | The user can select cloud use and control what is uploaded. | With cloud disabled, verify no remote call; with a selected scope, verify only allowed records are sent. |
| M08 | Preserve exported conversation identity, timestamps and original evidence. | Import fictional export samples twice and verify counts, chronology, stable IDs and deduplication. Report unsupported items. |

## Three comparison conditions

1. **Cloud archive only:** query the remote provider for every lookup.
2. **Cloud archive plus local catalog:** use a bounded local catalog and fetch source details from the archive. Verify behavior on misses and stale entries.
3. **Mem0 plus cloud-backed storage:** extract and retrieve memories with a versioned Mem0 configuration and the same fictional histories.

Hold the histories, questions, reader, grading and context budget fixed. Measure correctness and evidence coverage, cold/warm end-to-end and retrieval latency, local disk and RAM, cloud storage and calls, preparation and answer tokens, and actual billed cost when available. Record failures, cache misses, cloud configuration and privacy scope. Use [the report template](../reports/REPORT-TEMPLATE.md).

## Conversation exports

For an eligible personal account, use ChatGPT **Settings → Data controls → Export data**. The resulting ZIP includes conversation JSON. [OpenAI export instructions](https://help.openai.com/en/articles/7260999-exporting-your-chatgpt-history-and-data). Design a private, resumable importer around the actual export schema and the user's chosen cloud destination; preserve originals and avoid exposing private content in benchmark artifacts.

For an eligible individual Claude account, use **Settings → Privacy → Export data** on Claude web or desktop. Claude's export includes conversations, but not the original uploaded files. [Claude export instructions](https://support.claude.com/en/articles/9450526-export-your-claude-data). Both services provide time-limited download links; record imported and unsupported material without publishing the export.

## Current implementation

The first native reference catalog holds bounded in-memory metadata: record ID, person, memory category and tags. Its test covers category and person separation, replacement, capacity and removal. The capacity is an entry count, not a disk-byte limit, and caller-supplied person IDs do not provide authentication or authorization. It is a reference component; it does not persist to SQLite, build a vector index, connect to a cloud service, or implement the three measured comparisons yet. Firestore is a candidate to assess, not a configured backend. No real account data is used by the tests.

Firestore's [vector-search documentation](https://docs.cloud.google.com/firestore/native/docs/vector-search) describes flat indexes and currently lists Python, Node.js, Go and Java vector-search clients. Evaluate C++ access, configuration, costs, access controls and data export before choosing the adapter. A local catalog miss must not be interpreted as proof that the cloud archive lacks a record.
