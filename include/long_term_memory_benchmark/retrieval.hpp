#pragma once

#include "longmemeval.hpp"
#include <cstddef>
#include <map>
#include <set>
#include <string>
#include <string_view>
#include <vector>

namespace ltm::retrieval {

// A retrieval unit is one conversation round: a user message and the replies
// that follow it. It is identified by its user message's source ID.
struct Unit {
    std::string id;
    std::vector<std::string> source_ids;  // every message in the round, in order
    std::string text;                     // what retrieval scores
    std::string session_date;
    std::size_t round_bytes = 0;          // user message and replies, joined by newlines
};

enum class RoundText { user_only, user_and_replies };

// Rounds in chronological (input) order. Replies before a session's first user
// message have no round and are not retrievable.
std::vector<Unit> rounds(const CandidateCase& history, RoundText text);

// Lowercase ASCII letters and digits form terms; bytes >= 0x80 stay inside terms
// so UTF-8 words are kept whole. No stemming and no stop words.
std::vector<std::string> terms(std::string_view text);

struct Ranked {
    std::size_t unit;  // index into the ranked units
    double score;
};

// Okapi BM25 with Lucene's non-negative IDF: ln(1 + (N - n + 0.5) / (n + 0.5)).
// rank() orders every unit: higher score first, ties in input order.
class Bm25 {
public:
    explicit Bm25(const std::vector<Unit>& units, double k1 = 1.2, double b = 0.75);
    std::vector<Ranked> rank(std::string_view query) const;
private:
    double k1_, b_, average_length_ = 0;
    std::vector<std::map<std::string, std::size_t>> frequencies_;
    std::vector<std::size_t> lengths_;
    std::map<std::string, std::size_t> document_frequency_;
};

// Port: turns texts into embedding vectors, one per text, in order.
class Embedder {
public:
    virtual ~Embedder() = default;
    virtual std::vector<std::vector<float>> embed(const std::vector<std::string>& texts) = 0;
};

// Orders every unit vector by cosine similarity to the query; ties in input order.
std::vector<Ranked> cosine_rank(const std::vector<std::vector<float>>& units, const std::vector<float>& query);

// Reciprocal rank fusion: score = sum of 1 / (k + rank), ranks counted from 1.
// Every ranking must order the same units.
std::vector<Ranked> reciprocal_rank_fusion(const std::vector<std::vector<Ranked>>& rankings, double k = 60);

// Context-budget estimate used for every retriever: ceil(UTF-8 bytes / 4).
std::size_t estimated_tokens(std::size_t bytes);

// The longest prefix of the ranking whose rounds fit the token budget, returned
// as unit indices in chronological order. Stops at the first round that does not fit.
std::vector<std::size_t> pack(const std::vector<Unit>& units, const std::vector<Ranked>& ranking,
    std::size_t budget_tokens);

// Unit IDs in ranking order (the input the official LongMemEval scorer expects).
std::vector<std::string> ranked_ids(const std::vector<Unit>& units, const std::vector<Ranked>& ranking);

// Baseline: newest round first.
std::vector<Ranked> recency_rank(std::size_t count);

struct EvidenceCoverage {
    std::size_t evidence = 0;  // evidence rounds in the history
    std::size_t included = 0;  // of those, packed within the budget
};
// How many rounds whose ID is in evidence_ids fit in the budget.
EvidenceCoverage evidence_in_budget(const std::vector<Unit>& units, const std::vector<Ranked>& ranking,
    const std::set<std::string>& evidence_ids, std::size_t budget_tokens);

// The history reduced to the rounds packed within the budget (see pack), keeping
// session order and dates; sessions left empty are dropped. `selected` receives
// the kept message source IDs in order.
CandidateCase select_rounds(const CandidateCase& history, const std::vector<Unit>& units,
    const std::vector<Ranked>& ranking, std::size_t budget_tokens, std::vector<std::string>& selected);

// Ranks rounds for `method`: "bm25", "dense" (needs an embedder; the query is
// query_prefix + question) or "hybrid" (RRF of bm25 and dense).
std::vector<Ranked> rank_rounds(const std::string& method, const std::vector<Unit>& units,
    const std::string& question, Embedder* embedder, const std::string& query_prefix);

} // namespace ltm::retrieval
