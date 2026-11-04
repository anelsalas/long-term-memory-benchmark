# Research sources and code provenance

Terms are explained in the [glossary](../docs/GLOSSARY.md).

## Three foundation papers

1. **Di Wu, Hongwei Wang, Wenhao Yu, Yuwei Zhang, Kai-Wei Chang and Dong Yu.**
   *LongMemEval: Benchmarking Chat Assistants on Long-Term Interactive Memory.*
   ICLR 2025; arXiv v2, March 4, 2025; original submission October 14, 2024.
   [Paper](https://arxiv.org/abs/2410.10813v2) ·
   [Code and data instructions](https://github.com/xiaowu0162/LongMemEval).
   Supplies the primary benchmark, retrieval definitions and QA grading reference.

2. **Prateek Chhikara, Dev Khant, Saket Aryan, Taranjeet Singh and Deshraj Yadav.**
   *Mem0: Building Production-Ready AI Agents with Scalable Long-Term Memory.*
   arXiv v1, April 28, 2025.
   [Paper](https://arxiv.org/abs/2504.19413v1) ·
   [Memory implementation](https://github.com/mem0ai/mem0) ·
   [Current evaluation suite](https://github.com/mem0ai/memory-benchmarks).
   Supplies fact-memory and graph-memory comparisons on LoCoMo.

3. **Preston Rasmussen, Pavlo Paliychuk, Travis Beauvais, Jack Ryan and Daniel Chalef.**
   *Zep: A Temporal Knowledge Graph Architecture for Agent Memory.*
   arXiv v1, January 20, 2025.
   [Paper](https://arxiv.org/abs/2501.13956v1) ·
   [Graphiti implementation](https://github.com/getzep/graphiti).
   Supplies temporal-graph design and LongMemEval-S/DMR comparisons.

## Pinned source references

The initial C++ foundation is original project code. The papers supply research
methods and the references below identify code for subsequent adapter review.
Record attribution, revisions and notices with each incorporated upstream file.

These references identify selected upstream code for adaptation review. A run
manifest records the actual source, data and model revisions used in that run.

| Repository | Revision | Relevant paths |
| --- | --- | --- |
| [LongMemEval](https://github.com/xiaowu0162/LongMemEval/tree/9e0b455f4ef0e2ab8f2e582289761153549043fc) | `9e0b455f4ef0e2ab8f2e582289761153549043fc` | `src/retrieval/eval_utils.py`, `src/generation/run_generation.py`, `src/evaluation/evaluate_qa.py` |
| [Mem0 paper-era candidate](https://github.com/mem0ai/mem0/tree/42c98e571732268dd764db646c3faf8b6af62268/evaluation) | `42c98e571732268dd764db646c3faf8b6af62268` | `evaluation/src/rag.py`, `evaluation/src/memzero/`, `evaluation/metrics/llm_judge.py` |
| [Mem0 benchmark suite](https://github.com/mem0ai/memory-benchmarks/tree/4b61c5d31b9c668a12b4f5e78064248a02c82d2b) | `4b61c5d31b9c668a12b4f5e78064248a02c82d2b` | `benchmarks/longmemeval/`, `benchmarks/locomo/`, `benchmarks/common/` |
| [Graphiti](https://github.com/getzep/graphiti/tree/72b5c59c06607e09ea855be61b6ff5a3446fc951) | `72b5c59c06607e09ea855be61b6ff5a3446fc951` | `tests/evals/eval_e2e_graph_building.py`, `pyproject.toml` |

## Reproduction choices

- LongMemEval-S has 500 questions with histories averaging roughly 115,000 tokens;
  M histories are substantially larger. The original Table 3 retrieval study uses
  **M**. Record S development runs and M table reproduction as separate conditions.
- Pin original versus later cleaned data explicitly. Keep benchmark annotations
  in evaluation storage and apply the official retrieval exclusions and source mapping.
- The Mem0 paper uses LoCoMo. Its paper-era runner calls hosted Mem0 services and
  references formatted dataset files requiring a validated conversion. The dated
  candidate revision requires verification against the exact experimental snapshot.
- Current Mem0 evaluation defaults, prompts and question/cutoff selections differ
  from historical experiments. Declare reader, judge, workload and cutoffs explicitly.
- Zep evaluated a hosted AWS service. Current Graphiti is a versioned implementation
  candidate; reproducing the historical Zep arm requires matching its service and
  model configuration. The pinned Graphiti evaluation path scores graph construction.
- Preserve upstream notices and document every incorporated file, modification,
  dependency, model and dataset source. Review redistribution terms for each artifact.
  Cite algorithmic inspiration separately from reused implementation.

## Contamination research

- Shuo Yang et al. *Rethinking Benchmark and Contamination for Language Models with
  Rephrased Samples* (2023). [Paper](https://arxiv.org/abs/2311.04850).
- Jasper Dekoninck et al. *Evading Data Contamination Detection for Language Models
  is (too) Easy* (2024). [Paper](https://arxiv.org/abs/2402.02823).

Paper metadata was checked against primary arXiv records on October 31, 2026.
Recorded source revisions guide the first adaptation review; empirical runs and
measured comparisons are tracked through the run manifests and project status.
