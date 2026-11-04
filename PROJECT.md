# Project objective

Terms are explained in the [glossary](docs/GLOSSARY.md).

Build a public, reproducible benchmark for trustworthy, affordable personal memory.

1. Reproduce declared experiments from LongMemEval, Mem0 and Zep.
2. Document adaptations and compare methods under matched conditions.
3. Test new fictional facts, corrections, dates, identities and missing information.
4. Measure correctness, supporting evidence, speed, CPU, memory, storage, tokens
   and total inference cost with explicit measurement scopes.
5. Integrate fast component tests and selected experiments into applications through a
   versioned CMake dependency and a small memory adapter.

The Surface runs native ARM64 control, storage, retrieval and reporting. Model
adapters select local or remote inference for each declared experiment.
Published experiments and hidden final-evaluation stories form separate tracks.

## Milestones

- Native offline retrieval runner with behavioral tests and machine-readable output.
- One published question and one fresh synthetic case through a fixed model/scorer.
- Full declared benchmark, paired comparisons and reviewed public artifacts.
