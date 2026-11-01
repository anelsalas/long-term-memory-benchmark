"""Compare the C++ duplicate-ID score matrix to LongMemEval's evaluation code.

Pass the upstream eval_utils.py and ARM64 ltm_scoring_tests.exe paths. The
upstream module uses numpy only in dcg; use the equivalent standard-library
calculation here so the parity check requires only Python.
"""
import importlib.util
import json
import math
import subprocess
import sys
import types

if len(sys.argv) != 3:
    raise SystemExit("usage: check-duplicate-parity.py UPSTREAM_EVAL_UTILS SCORING_TEST_EXE")
sys.modules["numpy"] = types.ModuleType("numpy")
spec = importlib.util.spec_from_file_location("upstream_eval_utils", sys.argv[1])
upstream = importlib.util.module_from_spec(spec)
spec.loader.exec_module(upstream)
upstream.dcg = lambda values, k: sum(
    value / (1.0 if i == 0 else math.log2(i + 1))
    for i, value in enumerate(values[:k])
)
rows = json.loads(subprocess.check_output([sys.argv[2], "--duplicate-report"], text=True))
for row in rows:
    mode = row["mode"]
    labels = ({"s1": "answer_a", "s2": "answer_b", "s3": "answer_b"} if mode == 0 else
              {"m1": "answer_a_1", "m3": "noans_a_3", "m4": "answer_b_1", "m5": "answer_b_1"})
    ordered = sorted(labels)
    corpus = [labels[key] for key in ordered]
    ranking = [ordered.index(key) for key in row["rank"]]
    correct = list({value for value in corpus if "answer" in value})
    evaluate = upstream.evaluate_retrieval_turn2session if mode == 2 else upstream.evaluate_retrieval
    expected = evaluate(ranking, correct, corpus, k=row["k"])
    for name, value in zip(("any", "all", "ndcg"), expected):
        if abs(row[name] - value) > 1e-12:
            raise AssertionError((row, name, value))
print(f"PASS: {len(rows)} repeated-ID rankings and cutoffs agree with upstream")
