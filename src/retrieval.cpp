#include "long_term_memory_benchmark/retrieval.hpp"

#include <algorithm>
#include <cmath>
#include <set>
#include <stdexcept>

namespace ltm::retrieval {
namespace {
void by_score(std::vector<Ranked>& ranked) {
    std::stable_sort(ranked.begin(), ranked.end(), [](const Ranked& a, const Ranked& b) { return a.score > b.score; });
}
}
std::vector<Unit> rounds(const CandidateCase& history, RoundText text) {
    std::vector<Unit> units;
    for (const auto& session : history.sessions) {
        Unit* round = nullptr;
        for (const auto& message : session.messages) {
            if (message.role == "user") {
                units.push_back({message.source_id, {message.source_id}, message.content, session.date,
                    message.content.size()});
                round = &units.back();
            } else if (round) {
                round->source_ids.push_back(message.source_id);
                round->round_bytes += 1 + message.content.size();
                if (text == RoundText::user_and_replies) round->text += "\n" + message.content;
            }
        }
    }
    return units;
}

std::vector<std::string> terms(std::string_view text) {
    std::vector<std::string> result;
    std::string term;
    for (const unsigned char c : text) {
        if (c >= 'A' && c <= 'Z') term += static_cast<char>(c - 'A' + 'a');
        else if ((c >= 'a' && c <= 'z') || (c >= '0' && c <= '9') || c >= 0x80) term += static_cast<char>(c);
        else if (!term.empty()) result.push_back(std::move(term)), term.clear();
    }
    if (!term.empty()) result.push_back(std::move(term));
    return result;
}

Bm25::Bm25(const std::vector<Unit>& units, double k1, double b) : k1_(k1), b_(b) {
    std::size_t total = 0;
    for (const auto& unit : units) {
        auto& frequency = frequencies_.emplace_back();
        const auto words = terms(unit.text);
        for (const auto& word : words) ++frequency[word];
        for (const auto& entry : frequency) ++document_frequency_[entry.first];
        lengths_.push_back(words.size());
        total += words.size();
    }
    if (!units.empty()) average_length_ = static_cast<double>(total) / static_cast<double>(units.size());
}

std::vector<Ranked> Bm25::rank(std::string_view query) const {
    const auto count = static_cast<double>(frequencies_.size());
    const auto words = terms(query);
    std::vector<Ranked> ranked;
    for (std::size_t unit = 0; unit < frequencies_.size(); ++unit) {
        const auto norm = k1_ * (1 - b_ + b_ * static_cast<double>(lengths_[unit]) / average_length_);
        double score = 0;
        for (const auto& word : words) {
            const auto tf = frequencies_[unit].find(word);
            if (tf == frequencies_[unit].end()) continue;
            const auto n = static_cast<double>(document_frequency_.at(word));
            const auto f = static_cast<double>(tf->second);
            score += std::log(1 + (count - n + 0.5) / (n + 0.5)) * f * (k1_ + 1) / (f + norm);
        }
        ranked.push_back({unit, score});
    }
    by_score(ranked);
    return ranked;
}


std::vector<Ranked> cosine_rank(const std::vector<std::vector<float>>& units, const std::vector<float>& query) {
    double query_norm = 0;
    for (const auto x : query) query_norm += static_cast<double>(x) * x;
    std::vector<Ranked> ranked;
    for (std::size_t unit = 0; unit < units.size(); ++unit) {
        if (units[unit].size() != query.size()) throw std::invalid_argument("Embedding dimensions differ");
        double dot = 0, norm = 0;
        for (std::size_t i = 0; i < query.size(); ++i) {
            dot += static_cast<double>(units[unit][i]) * query[i];
            norm += static_cast<double>(units[unit][i]) * units[unit][i];
        }
        ranked.push_back({unit, norm == 0 || query_norm == 0 ? 0 : dot / std::sqrt(norm * query_norm)});
    }
    by_score(ranked);
    return ranked;
}

std::vector<Ranked> reciprocal_rank_fusion(const std::vector<std::vector<Ranked>>& rankings, double k) {
    if (rankings.empty()) return {};
    const auto size = rankings.front().size();
    std::vector<double> scores(size, 0);
    std::vector<std::size_t> seen(size, 0);
    for (const auto& ranking : rankings) {
        if (ranking.size() != size) throw std::invalid_argument("Rankings must order the same units");
        for (std::size_t rank = 0; rank < size; ++rank) {
            const auto unit = ranking[rank].unit;
            if (unit >= size || ++seen[unit] > rankings.size()) throw std::invalid_argument("Rankings must order the same units");
            scores[unit] += 1 / (k + static_cast<double>(rank + 1));
        }
    }
    std::vector<Ranked> fused;
    for (std::size_t unit = 0; unit < size; ++unit) {
        if (seen[unit] != rankings.size()) throw std::invalid_argument("Rankings must order the same units");
        fused.push_back({unit, scores[unit]});
    }
    by_score(fused);
    return fused;
}

std::size_t estimated_tokens(std::size_t bytes) { return (bytes + 3) / 4; }

std::vector<std::size_t> pack(const std::vector<Unit>& units, const std::vector<Ranked>& ranking,
    std::size_t budget_tokens) {
    std::vector<std::size_t> selected;
    std::size_t used = 0;
    for (const auto& ranked : ranking) {
        const auto cost = estimated_tokens(units.at(ranked.unit).round_bytes);
        if (used + cost > budget_tokens) break;
        used += cost;
        selected.push_back(ranked.unit);
    }
    std::sort(selected.begin(), selected.end());
    return selected;
}

std::vector<std::string> ranked_ids(const std::vector<Unit>& units, const std::vector<Ranked>& ranking) {
    std::vector<std::string> ids;
    for (const auto& ranked : ranking) ids.push_back(units.at(ranked.unit).id);
    return ids;
}

std::vector<Ranked> recency_rank(std::size_t count) {
    std::vector<Ranked> ranked;
    for (std::size_t unit = count; unit-- > 0;) ranked.push_back({unit, static_cast<double>(unit)});
    return ranked;
}

EvidenceCoverage evidence_in_budget(const std::vector<Unit>& units, const std::vector<Ranked>& ranking,
    const std::set<std::string>& evidence_ids, std::size_t budget_tokens) {
    EvidenceCoverage coverage;
    for (const auto& unit : units) coverage.evidence += evidence_ids.count(unit.id);
    for (const auto index : pack(units, ranking, budget_tokens)) coverage.included += evidence_ids.count(units[index].id);
    return coverage;
}

CandidateCase select_rounds(const CandidateCase& history, const std::vector<Unit>& units,
    const std::vector<Ranked>& ranking, std::size_t budget_tokens, std::vector<std::string>& selected) {
    std::set<std::string> keep;
    for (const auto index : pack(units, ranking, budget_tokens))
        keep.insert(units[index].source_ids.begin(), units[index].source_ids.end());
    auto reduced = history;
    reduced.sessions.clear();
    for (const auto& session : history.sessions) {
        auto kept = session;
        kept.messages.clear();
        for (const auto& message : session.messages) {
            if (!keep.count(message.source_id)) continue;
            kept.messages.push_back(message);
            selected.push_back(message.source_id);
        }
        if (!kept.messages.empty()) reduced.sessions.push_back(std::move(kept));
    }
    return reduced;
}

} // namespace ltm::retrieval
