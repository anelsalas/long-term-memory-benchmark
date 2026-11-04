# Retrieval baseline preparation — Codex summary, November 1, 2026

Terms are explained in the [glossary](../docs/GLOSSARY.md).

The cleaned LongMemEval-S dataset contains 500 cases. Source: https://huggingface.co/datasets/xiaowu0162/longmemeval-cleaned/tree/98d7416c24c778c2fee6e6f3006e7a073259d48f . The `longmemeval_s_cleaned.json` file has 277,383,467 bytes and SHA-256 `d6f21ea9d60a0d56f34a05b609c79c88a451d2ae03597821ea3d5a9678c3a442`.

The development selection uses seed `42`: sort question IDs per type by the bytes of SHA-256 of `42:<question_type>:<question_id>`, reserve the first `_abs` ID when available, and fill to ten with the remaining ranked IDs. The 60 selected IDs cover six types, including five abstention cases. [Selected IDs](../configs/longmemeval-dev-60-seed42.json). The dataset and model are stored in local runtime storage.

The local embedding endpoint uses Qwen3-Embedding-0.6B Q8_0 GGUF, model revision `370f27d7550e0def9b39c1f16d3fbaa13aa67728`, SHA-256 `06507c7b42688469c4e7298b0a1e16deff06caf291cf0a5b278c308249c3e439`; llama.cpp revision `620d2225cc436ab0b001e3200e590e3b286ef9ab`, Windows ARM64 CPU, two threads, last-token pooling and L2 normalization. Context, batch and microbatch sizes are 8,192. A live four-short-text test returned four ordered 1,024-dimensional vectors with L2 norms within 0.000001 of 1. A warm repetition took 1.708 seconds, or 2.34 texts/s; throughput for longer inputs remains to be measured.

The adapted development test prepares a reusable retrieval comparison; results remain to be measured.
