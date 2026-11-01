# Codex dataset occurrence correction — November 1, 2026

Summary: added behavioral tests, observed the old loader/scorer fail, then implemented stable date ordering and support for repeated original session IDs with distinct opaque occurrences. ARM64 loader/scorer tests passed; three affected published cases passed schema validation.

Next: check duplicate-ID scorer parity with upstream and explicitly evaluate how to handle sessions after question dates. Claude owns compare_main.cpp. The retrieval branch requires independent integration review.
