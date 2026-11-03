# Summary: per-request instructions, November 3, 2026

The bridge accepts a leading system message separately from a user message.
The Codex runner creates a temporary instruction file for each attempt and
removes it when the runner returns. The test first failed to build without the
new interface, then passed; a child-process fixture checked file contents and
cleanup. The full Windows ARM64 offline suite passed 110/110. No model calls.

Abrupt termination can leave temporary files; interruption cleanup and production
privacy protections need further validation before real personal data is used.
