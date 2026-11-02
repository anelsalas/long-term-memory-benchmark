#include "long_term_memory_benchmark/retrieval_eval.hpp"

#include <set>
#include <utility>

namespace ltm::retrieval {
namespace {
std::set<std::string> evidence_rounds(const LoadedLongMemEvalCase& data) {
    std::set<std::string> user_messages;
    for (const auto& session : data.candidate.sessions)
        for (const auto& message : session.messages)
            if (message.role == "user") user_messages.insert(message.source_id);
    std::set<std::string> evidence;
    for (const auto& message : data.evaluator.messages)
        if (message.has_answer.value_or(false) && user_messages.count(message.source_id)) evidence.insert(message.source_id);
    return evidence;
}
}

std::vector<MethodResult> evaluate_case(const LoadedLongMemEvalCase& data, Embedder* embedder,
    const std::string& query_prefix, const std::vector<std::size_t>& budgets) {
    const auto units = rounds(data.candidate, RoundText::user_only);
    std::vector<std::pair<std::string, std::vector<Ranked>>> methods;
    methods.emplace_back("recency", recency_rank(units.size()));
    for (const auto* method : {"bm25", "dense", "hybrid"}) {
        if (std::string(method) != "bm25" && !embedder) break;
        methods.emplace_back(method, rank_rounds(method, units, data.candidate.question, embedder, query_prefix));
    }
    const auto evidence = evidence_rounds(data);
    std::vector<MethodResult> results;
    for (const auto& [name, ranking] : methods) {
        const auto ids = ranked_ids(units, ranking);
        MethodResult result{name,
            score_official_retrieval(data, ids, 5, RetrievalLevel::turn),
            score_official_retrieval(data, ids, 10, RetrievalLevel::turn),
            score_official_retrieval(data, ids, 5, RetrievalLevel::turn_to_session), {}};
        for (const auto budget : budgets) result.budgets.push_back(evidence_in_budget(units, ranking, evidence, budget));
        results.push_back(std::move(result));
    }
    return results;
}

CoverageSummary summarize_coverage(const std::vector<EvidenceCoverage>& coverage) {
    CoverageSummary summary;
    double fraction = 0, complete = 0;
    for (const auto& item : coverage) {
        if (item.evidence == 0) continue;
        ++summary.questions;
        fraction += static_cast<double>(item.included) / static_cast<double>(item.evidence);
        complete += item.included == item.evidence ? 1 : 0;
    }
    if (summary.questions == 0) return summary;
    summary.mean_fraction = fraction / static_cast<double>(summary.questions);
    summary.all_included_rate = complete / static_cast<double>(summary.questions);
    return summary;
}


std::size_t sessions_after_question(const CandidateCase& history) {
    const auto key = [](const std::string& date) {
        return date.size() < 16 ? date : date.substr(0, 10) + date.substr(date.size() - 5);
    };
    const auto question = key(history.question_date);
    std::size_t later = 0;
    for (const auto& session : history.sessions) later += key(session.date) > question;
    return later;
}

} // namespace ltm::retrieval
