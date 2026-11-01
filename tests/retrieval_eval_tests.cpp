#include "long_term_memory_benchmark/hash.hpp"
#include "long_term_memory_benchmark/retrieval_eval.hpp"
#include <cmath>
#include <fstream>
#include <functional>
#include <iostream>
#include <iterator>
#include <stdexcept>
#include <string>
#include <utility>
#include <vector>

namespace {
using namespace ltm::retrieval;

void require(bool condition, const std::string& message) {
    if (!condition) throw std::runtime_error(message);
}
ltm::LoadedLongMemEvalCase fixture(const std::string& question_id) {
    std::ifstream in(LTM_FIXTURE_PATH, std::ios::binary);
    const std::string bytes((std::istreambuf_iterator<char>(in)), {});
    return ltm::load_longmemeval_case(bytes, question_id, {"fixture", "test", ltm::sha256(bytes)});
}
struct FakeEmbedder : Embedder {
    std::vector<std::string> seen;
    std::vector<std::vector<float>> embed(const std::vector<std::string>& texts) override {
        std::vector<std::vector<float>> vectors;
        for (const auto& text : texts) {
            seen.push_back(text);
            vectors.push_back({text.find("corrected") != std::string::npos ? 1.0f : 0.0f, 1.0f});
        }
        return vectors;
    }
};
std::vector<std::string> names(const std::vector<MethodResult>& results) {
    std::vector<std::string> out;
    for (const auto& r : results) out.push_back(r.method);
    return out;
}

void lexical_methods_score_evidence() {
    const auto results = evaluate_case(fixture("fictional-school-01"), nullptr, "", {10, 1000});
    require(names(results) == std::vector<std::string>{"recency", "bm25"}, "Without an embedder: recency and BM25");
    const auto& bm25 = results[1];
    require(bm25.turn_at_5.metrics && bm25.turn_at_5.metrics->recall_all == 1, "Both evidence turns are in the top 5");
    require(bm25.session_at_5.metrics && bm25.session_at_5.metrics->recall_all == 1, "Session-level scoring works");
    require(bm25.budgets.size() == 2 && bm25.budgets[1].evidence == 2 && bm25.budgets[1].included == 2, "A large budget covers both");
    require(bm25.budgets[0].included < 2, "A 10-token budget cannot hold both evidence rounds");
}
void dense_and_hybrid_use_the_embedder() {
    FakeEmbedder embedder;
    const auto results = evaluate_case(fixture("fictional-school-01"), &embedder, "Query: ", {1000});
    require(names(results) == std::vector<std::string>{"recency", "bm25", "dense", "hybrid"}, "Dense and hybrid added");
    require(embedder.seen.back() == "Query: What is Lina's corrected astronomy project score?", "Query carries the prefix");
    require(results[2].turn_at_5.metrics && results[3].budgets[0].included == 2, "Dense and hybrid are scored");
}
void abstention_is_excluded() {
    const auto results = evaluate_case(fixture("fictional-school-02_abs"), nullptr, "", {1000});
    require(results[1].turn_at_5.exclusion == ltm::RetrievalExclusion::abstention && !results[1].turn_at_5.metrics,
        "Abstention questions have no retrieval metrics");
    require(results[1].budgets[0].evidence == 0, "Abstention questions have no evidence rounds");
}
void coverage_summary() {
    const auto summary = summarize_coverage({{2, 2}, {4, 1}, {0, 0}, {1, 0}});
    require(summary.questions == 3, "Questions without evidence are excluded");
    require(summary.mean_fraction && std::abs(*summary.mean_fraction - 1.25 / 3) < 1e-12, "Mean of 1, 0.25 and 0");
    require(summary.all_included_rate && std::abs(*summary.all_included_rate - 1.0 / 3) < 1e-12, "One of three fully covered");
    require(!summarize_coverage({{0, 0}}).mean_fraction, "No eligible questions, no mean");
}
}

int main(int argc, char** argv) {
    const std::vector<std::pair<std::string, std::function<void()>>> cases = {
        {"lexical_methods_score_evidence", lexical_methods_score_evidence},
        {"dense_and_hybrid_use_the_embedder", dense_and_hybrid_use_the_embedder},
        {"abstention_is_excluded", abstention_is_excluded}, {"coverage_summary", coverage_summary}};
    if (argc != 2) return 2;
    for (const auto& item : cases) {
        if (item.first == argv[1]) {
            try { item.second(); std::cout << "PASS " << item.first << '\n'; return 0; }
            catch (const std::exception& e) { std::cerr << "FAIL " << item.first << ": " << e.what() << '\n'; return 1; }
        }
    }
    return 2;
}
