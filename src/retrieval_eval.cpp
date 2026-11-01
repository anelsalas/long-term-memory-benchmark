#include "long_term_memory_benchmark/retrieval_eval.hpp"

#include <nlohmann/json.hpp>
#include <algorithm>
#include <numeric>
#include <set>
#include <stdexcept>
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
    methods.emplace_back("bm25", Bm25(units).rank(data.candidate.question));
    if (embedder) {
        std::vector<std::string> texts;
        for (const auto& unit : units) texts.push_back(unit.text);
        texts.push_back(query_prefix + data.candidate.question);
        auto vectors = embedder->embed(texts);
        const auto query = std::move(vectors.back());
        vectors.pop_back();
        methods.emplace_back("dense", cosine_rank(vectors, query));
        methods.emplace_back("hybrid", reciprocal_rank_fusion({methods[1].second, methods[2].second}));
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

std::string sort_sessions_by_date(const std::string& entry_json, bool& moved) {
    auto entry = nlohmann::json::parse(entry_json);
    const auto& dates = entry.at("haystack_dates");
    const auto key = [&](std::size_t i) {
        const auto date = dates.at(i).get<std::string>();
        if (date.size() < 16) throw std::invalid_argument("Unsupported history date: " + date);
        return date.substr(0, 10) + date.substr(date.size() - 5);
    };
    std::vector<std::size_t> order(dates.size());
    std::iota(order.begin(), order.end(), std::size_t{0});
    std::stable_sort(order.begin(), order.end(), [&](std::size_t a, std::size_t b) { return key(a) < key(b); });
    moved = !std::is_sorted(order.begin(), order.end());
    if (!moved) return entry_json;
    for (const auto* field : {"haystack_sessions", "haystack_dates", "haystack_session_ids"}) {
        const auto original = entry.at(field);
        if (original.size() != order.size()) throw std::invalid_argument("History fields differ in length");
        auto& target = entry[field];
        for (std::size_t i = 0; i < order.size(); ++i) target[i] = original[order[i]];
    }
    return entry.dump();
}

} // namespace ltm::retrieval
