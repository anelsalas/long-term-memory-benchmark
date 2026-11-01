# Codex duplicate-ID scoring parity — November 1, 2026

Summary: compared a synthetic repeated-ID fixture across 162 rankings/cutoffs and three scoring levels against upstream retrieval evaluation functions from revision 9e0b455f4ef0e2ab8f2e582289761153549043fc. All matched; NumPy DCG was replaced with an equivalent standard-library calculation. Both ARM64 loader/scorer CTest groups passed.

Next: validate a policy for post-question timestamps and review Claude's retrieval branch. Claude owns compare_main.cpp.
