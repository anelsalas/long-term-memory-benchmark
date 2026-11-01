#include "long_term_memory_benchmark/retrieval.hpp"
#include <cmath>
#include <functional>
#include <iostream>
#include <stdexcept>
#include <string>
#include <utility>
#include <vector>

namespace {
using namespace ltm::retrieval;
using Ids = std::vector<std::string>;

void require(bool condition, const std::string& message) {
    if (!condition) throw std::runtime_error(message);
}
ltm::CandidateCase history() {
    ltm::CandidateCase c;
    c.question = "What grade did Lina get?";
    c.sessions = {
        {"s1", "2026/01/05 (Mon) 09:00", {{"a0", "assistant", "Welcome back."},
            {"u1", "user", "Lina got 23 on the chemistry test."}, {"a1", "assistant", "Noted."},
            {"u2", "user", "The weather is nice."}}},
        {"s2", "2026/01/09 (Fri) 10:00", {{"u3", "user", "Correction: Lina got 82, not 23."},
            {"a3", "assistant", "Updated."}, {"a4", "assistant", "Anything else?"}}}};
    return c;
}

void rounds_group_user_with_replies() {
    const auto units = rounds(history(), RoundText::user_and_replies);
    require(units.size() == 3, "One round per user message");
    require(units[0].id == "u1" && units[0].source_ids == Ids{"u1", "a1"}, "A round holds the user message and its reply");
    require(units[1].id == "u2" && units[1].source_ids == Ids{"u2"}, "A round may have no reply");
    require(units[2].source_ids == Ids{"u3", "a3", "a4"}, "Every reply until the next user message joins the round");
    require(units[0].text == "Lina got 23 on the chemistry test.\nNoted.", "Round text joins messages by line");
    require(units[2].session_date == "2026/01/09 (Fri) 10:00", "A round keeps its session date");
}
void rounds_score_user_text_only() {
    const auto units = rounds(history(), RoundText::user_only);
    require(units.size() == 3 && units[2].text == "Correction: Lina got 82, not 23.", "User-only rounds score user text");
    require(units[2].source_ids == Ids{"u3", "a3", "a4"}, "User-only rounds still carry their replies");
    require(units[0].round_bytes == std::string("Lina got 23 on the chemistry test.\nNoted.").size(), "Round size counts replies");
}
std::vector<Unit> corpus(const std::vector<std::string>& texts) {
    std::vector<Unit> units;
    for (std::size_t i = 0; i < texts.size(); ++i) units.push_back({"u" + std::to_string(i), {}, texts[i], ""});
    return units;
}
bool near(double a, double b) { return std::fabs(a - b) < 1e-5; }

void terms_fold_case_and_split() {
    require(terms("Lina's GRADE: 82!") == Ids{"lina", "s", "grade", "82"}, "ASCII letters and digits form lowercase terms");
    require(terms("caf\xC3\xA9 menu") == Ids{"caf\xC3\xA9", "menu"}, "UTF-8 bytes stay inside a term");
    require(terms(" .,; ").empty(), "Punctuation alone has no terms");
}
void bm25_matches_okapi() {
    const auto units = corpus({"lina got 23", "lina got 82 correction", "weather nice"});
    const auto ranked = Bm25(units).rank("Lina correction");
    require(ranked.size() == 3, "Every unit is ranked");
    require(ranked[0].unit == 1 && near(ranked[0].score, 1.276733), "Rare matching terms rank first");
    require(ranked[1].unit == 0 && near(ranked[1].score, 0.470004), "Length-normalized partial match second");
    require(ranked[2].unit == 2 && ranked[2].score == 0, "Units without matches rank last with score zero");
}
void bm25_ties_keep_input_order() {
    const auto ranked = Bm25(corpus({"red", "blue", "red"})).rank("green");
    require(ranked.size() == 3 && ranked[0].unit == 0 && ranked[1].unit == 1 && ranked[2].unit == 2, "Ties stay in input order");
    require(Bm25({}).rank("red").empty(), "An empty corpus ranks nothing");
}
std::vector<std::size_t> order(const std::vector<Ranked>& ranked) {
    std::vector<std::size_t> units;
    for (const auto& r : ranked) units.push_back(r.unit);
    return units;
}
template<class F> bool rejected(F action) {
    try { action(); } catch (const std::invalid_argument&) { return true; }
    return false;
}
void cosine_orders_by_similarity() {
    const auto ranked = cosine_rank({{0, 2}, {0.6f, 0.8f}, {3, 0}, {0, 0}}, {2, 0});
    require(order(ranked) == std::vector<std::size_t>{2, 1, 0, 3}, "Most similar direction first, zero vector last");
    require(near(ranked[0].score, 1) && near(ranked[1].score, 0.6) && ranked[3].score == 0, "Scores are cosines");
    require(rejected([] { cosine_rank({{1, 0}, {1, 0, 0}}, {1, 0}); }), "Mismatched dimensions are rejected");
}
void rrf_fuses_rankings() {
    const std::vector<Ranked> a{{0, 9}, {1, 5}, {2, 1}}, b{{2, 0.9}, {0, 0.5}, {1, 0.1}};
    const auto fused = reciprocal_rank_fusion({a, b});
    require(order(fused) == std::vector<std::size_t>{0, 2, 1}, "Consistently high ranks win");
    require(near(fused[0].score, 1.0 / 61 + 1.0 / 62) && near(fused[2].score, 1.0 / 62 + 1.0 / 63), "Scores sum 1/(60+rank)");
    require(rejected([&] { reciprocal_rank_fusion({a, {{0, 1}, {1, 1}}}); }), "Rankings of different units are rejected");
}
void pack_fills_budget_in_rank_order() {
    std::vector<Unit> units(4);
    const std::size_t bytes[] = {40, 37, 100, 8};
    for (std::size_t i = 0; i < 4; ++i) units[i].round_bytes = bytes[i];
    require(estimated_tokens(37) == 10 && estimated_tokens(40) == 10 && estimated_tokens(0) == 0, "Tokens are ceil(bytes / 4)");
    const std::vector<Ranked> ranking{{2, 4}, {0, 3}, {1, 2}, {3, 1}};
    require(pack(units, ranking, 30) == std::vector<std::size_t>{2}, "Stops at the first round that does not fit");
    require(pack(units, ranking, 45) == std::vector<std::size_t>{0, 1, 2}, "Selected rounds return in chronological order");
    require(pack(units, ranking, 1000) == std::vector<std::size_t>{0, 1, 2, 3}, "A large budget takes every round");
    require(pack(units, ranking, 0).empty(), "A zero budget selects nothing");
}
void evaluation_helpers() {
    auto units = corpus({"a", "b", "c"});
    for (auto& unit : units) unit.round_bytes = 40;  // 10 tokens each
    const std::vector<Ranked> ranking{{2, 3}, {0, 2}, {1, 1}};
    require(ranked_ids(units, ranking) == Ids{"u2", "u0", "u1"}, "IDs follow the ranking");
    require(order(recency_rank(3)) == std::vector<std::size_t>{2, 1, 0}, "Recency ranks the newest round first");
    const auto coverage = evidence_in_budget(units, ranking, {"u0", "u1", "missing"}, 20);
    require(coverage.evidence == 2 && coverage.included == 1, "Counts evidence rounds present and packed");
    require(evidence_in_budget(units, ranking, {}, 20).evidence == 0, "No evidence, nothing to cover");
}
}

int main(int argc, char** argv) {
    const std::vector<std::pair<std::string, std::function<void()>>> cases = {
        {"rounds_group_user_with_replies", rounds_group_user_with_replies},
        {"rounds_score_user_text_only", rounds_score_user_text_only},
        {"terms_fold_case_and_split", terms_fold_case_and_split}, {"bm25_matches_okapi", bm25_matches_okapi},
        {"bm25_ties_keep_input_order", bm25_ties_keep_input_order},
        {"cosine_orders_by_similarity", cosine_orders_by_similarity}, {"rrf_fuses_rankings", rrf_fuses_rankings},
        {"pack_fills_budget_in_rank_order", pack_fills_budget_in_rank_order},
        {"evaluation_helpers", evaluation_helpers}};
    if (argc != 2) return 2;
    for (const auto& item : cases) {
        if (item.first == argv[1]) {
            try { item.second(); std::cout << "PASS " << item.first << '\n'; return 0; }
            catch (const std::exception& e) { std::cerr << "FAIL " << item.first << ": " << e.what() << '\n'; return 1; }
        }
    }
    return 2;
}
