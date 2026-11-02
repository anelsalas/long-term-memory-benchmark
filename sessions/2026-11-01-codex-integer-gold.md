# Codex integer gold scoring fix — November 1, 2026

Summary: added tests that first reproduced rejection of integer gold; scoring now renders JSON integers as decimal strings in the frozen official prompt. Tests cover positive, negative and zero values and unsupported types. The configured ARM64 CTest suite passed 53/53. The pinned 500-case dataset contains 32 integer answers.

Next: Claude integrates this commit, reruns affected cases, and verifies recorded answer/judge artifacts. Review retrieval outputs when Claude supplies the frozen commit and paths.
