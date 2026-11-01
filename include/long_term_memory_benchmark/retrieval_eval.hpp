#pragma once

#include "retrieval.hpp"
#include "scoring.hpp"
#include <cstddef>
#include <string>
#include <vector>

namespace ltm::retrieval {

struct MethodResult {
    std::string method;                        // recency, bm25, dense or hybrid
    OfficialRetrievalScore turn_at_5, turn_at_10, session_at_5;
    std::vector<EvidenceCoverage> budgets;     // one per requested budget, same order
};

// Ranks the question's user-only rounds with recency and BM25, and with dense and
// hybrid (RRF of BM25 and dense) when an embedder is given. The dense query is
// query_prefix + question. Evidence rounds are user messages labelled has_answer.
std::vector<MethodResult> evaluate_case(const LoadedLongMemEvalCase& data, Embedder* embedder,
    const std::string& query_prefix, const std::vector<std::size_t>& budgets);

} // namespace ltm::retrieval
