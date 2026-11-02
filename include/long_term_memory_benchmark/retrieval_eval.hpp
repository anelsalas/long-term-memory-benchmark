#pragma once

#include "retrieval.hpp"
#include "scoring.hpp"
#include <cstddef>
#include <optional>
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

struct CoverageSummary {
    std::size_t questions = 0;                // questions with at least one evidence round
    std::optional<double> mean_fraction;      // mean share of each question's evidence packed
    std::optional<double> all_included_rate;  // share of questions with all evidence packed
};
CoverageSummary summarize_coverage(const std::vector<EvidenceCoverage>& coverage);

// Sessions dated after the question (dates as "YYYY/MM/DD (Day) HH:MM"). Nonzero only when
// the history was loaded with HistoryTimePolicy::include_all.
std::size_t sessions_after_question(const CandidateCase& history);

} // namespace ltm::retrieval
