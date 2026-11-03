# Summary — Mem0 context-budget correction

Claude identified excess prompt framing in the first Mem0 comparison. Codex changed the memory condition to render dated plain-text lines, count the complete block and preserve provenance outside the prompt. A behavioral test was added first; 118 CTest checks and two focused checks passed on Windows ARM64. The corrected reader and judge run is pending.
