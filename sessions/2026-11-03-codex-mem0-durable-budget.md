# Summary: durable call budget, November 3, 2026

Added a locked, flushed Windows attempt ledger for the CLI bridge. It preserves
the cap across restarts, refuses changed limits and malformed ledgers, and was
tested with concurrent handles and a restarted bridge. The Windows ARM64 offline
C++ suite passed 118/118; the prior Node suite passed 9/9. No live model calls.
