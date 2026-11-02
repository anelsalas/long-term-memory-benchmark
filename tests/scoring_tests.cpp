#include "long_term_memory_benchmark/scoring.hpp"
#include "long_term_memory_benchmark/hash.hpp"
#include <nlohmann/json.hpp>
#include <algorithm>
#include <cmath>
#include <functional>
#include <iostream>
#include <stdexcept>
#include <string>

namespace {
void require(bool value, const char* message) { if (!value) throw std::runtime_error(message); }
void close(double actual, double expected) { require(std::abs(actual - expected) < 1e-12, "Numeric parity"); }
ltm::LoadedLongMemEvalCase sample() {
    ltm::LoadedLongMemEvalCase result;
    result.candidate = {"case-1", "What was the revised score?", "2026/03/13 (Fri) 10:00", {
        {"s1", "2026/03/01 (Sun) 10:00", {{"m1", "user", "First score."}, {"m2", "assistant", "Recorded."}, {"m3", "user", "Other discussion."}}},
        {"s2", "2026/03/02 (Mon) 10:00", {{"m4", "user", "Revision."}}},
        {"s3", "2026/03/03 (Tue) 10:00", {{"m5", "user", "Unrelated."}}}}};
    result.evaluator = {"q1", "knowledge-update", false, "\"EVALUATOR_GOLD\"", {"answer_a", "answer_b"},
        {{"s1", "answer_a"}, {"s2", "answer_b"}, {"s3", "background_c"}},
        {{"m1", "answer_a", 1, true, "{}"}, {"m2", "answer_a", 2, std::nullopt, "{}"},
         {"m3", "answer_a", 3, false, "{}"}, {"m4", "answer_b", 1, true, "{}"},
         {"m5", "background_c", 1, std::nullopt, "{}"}}};
    return result;
}
void invalid(const std::function<void()>& action) {
    bool rejected = false; try { action(); } catch (const std::invalid_argument&) { rejected = true; }
    require(rejected, "Invalid scorer input rejected");
}
void sessions() {
    auto data = sample();
    const auto result = ltm::score_official_retrieval(data, {"s1", "s3", "s2"}, 2, ltm::RetrievalLevel::session);
    require(result.metrics.has_value(), "Eligible score");
    close(result.metrics->recall_any, 1); close(result.metrics->recall_all, 0); close(result.metrics->ndcg, 0.5);
    const auto all = ltm::score_official_retrieval(data, {"s1", "s3", "s2"}, 3, ltm::RetrievalLevel::session);
    close(all.metrics->ndcg, 0.8154648767857288);
    close(all.metrics->recall_all, 1);
}
void turn_mapping() {
    auto data = sample(); data.evaluator.messages[2].has_answer = true;
    const auto result = ltm::score_official_retrieval(data, {"m1", "m3", "m5", "m4"}, 2, ltm::RetrievalLevel::turn_to_session);
    require(result.metrics->effective_k == 3, "Effective k counts distinct rewritten sessions");
    close(result.metrics->recall_any, 1); close(result.metrics->recall_all, 0);
    close(result.metrics->ndcg, 2.0 / (2.0 + 1.0 / std::log2(3.0)));
    const auto turn = ltm::score_official_retrieval(data, {"m1", "m3", "m5", "m4"}, 2, ltm::RetrievalLevel::turn);
    close(turn.metrics->recall_all, 0);
}
void exclusions() {
    auto data = sample(); data.evaluator.question_id = "q_abs_extra";
    require(ltm::score_official_retrieval(data, {}, 2, ltm::RetrievalLevel::session).exclusion == ltm::RetrievalExclusion::abstention, "Official contains-abstention exclusion");
    data = sample(); for (auto& message : data.evaluator.messages) message.has_answer = false;
    const auto result = ltm::score_official_retrieval(data, {}, 2, ltm::RetrievalLevel::session);
    require(!result.metrics && result.exclusion == ltm::RetrievalExclusion::no_user_answer_label, "No user target excluded");
    const auto aggregate = ltm::aggregate_official_retrieval({"q1", "missing"}, {result});
    require(aggregate.expected == 2 && aggregate.no_user_answer_label == 1 && aggregate.missing == 1 && !aggregate.mean_ndcg, "Zero eligible aggregate unavailable");
}
void bad_rankings() {
    const auto data = sample();
    invalid([&] { (void)ltm::score_official_retrieval(data, {"s1", "s1", "s2"}, 1, ltm::RetrievalLevel::session); });
    invalid([&] { (void)ltm::score_official_retrieval(data, {"s1", "unknown", "s2"}, 1, ltm::RetrievalLevel::session); });
    invalid([&] { (void)ltm::score_official_retrieval(data, {"s1"}, 1, ltm::RetrievalLevel::session); });
    invalid([&] { (void)ltm::score_official_retrieval(data, {"s1", "s2", "s3"}, 0, ltm::RetrievalLevel::session); });
    auto bad = data; bad.evaluator.messages[0].original_message_number = 0;
    invalid([&] { (void)ltm::score_official_retrieval(bad, {"s1", "s2", "s3"}, 1, ltm::RetrievalLevel::session); });
    bad = data; bad.evaluator.messages[0].has_answer.reset();
    invalid([&] { (void)ltm::score_official_retrieval(bad, {"s1", "s2", "s3"}, 1, ltm::RetrievalLevel::session); });
}
void repeated_source_ids() {
    auto data = sample();
    data.evaluator.sessions[2].original_session_id = "answer_b";
    data.evaluator.messages[4].original_session_id = "answer_b";
    data.evaluator.messages[4].has_answer = false;
    const auto result = ltm::score_official_retrieval(data, {"s1", "s2", "s3"}, 2, ltm::RetrievalLevel::session);
    require(result.metrics.has_value(), "Repeated original identity scores by occurrence");
    close(result.metrics->recall_all, 1);
}
void grade_prompts() {
    auto data = sample();
    const auto candidate = ltm::candidate_input_json(data.candidate);
    const auto request = ltm::make_official_grade_request(data, "frozen response");
    require(request.prompt.find("EVALUATOR_GOLD") != std::string::npos, "Gold enters isolated judge request");
    require(ltm::candidate_input_json(data.candidate) == candidate && candidate.find("EVALUATOR_GOLD") == std::string::npos, "Candidate remains gold-free");
    require(request.prompt_sha256 == ltm::sha256(request.prompt), "Exact prompt hashed");
    require(request.template_sha256.size() == 64, "Frozen template identity recorded");
    require(request.template_sha256 == "183a9b3a6197ec620940f610cdc1207201ec98c1113dd633ea685cfc322fafac", "Pinned update template hash");
    require(request.temperature == 0 && request.max_output_tokens == 10, "Original grading parameters");
    require(request.message_role == "user" && request.sample_count == 1, "Original single user-message request");
    data.evaluator.question_type = "temporal-reasoning";
    require(ltm::make_official_grade_request(data, "x").template_sha256 == "8d33a5fdd83afeeb4592454a965eab43d1fcb2dedc042d1d3892f4254be6c273", "Pinned temporal template hash");
    require(ltm::make_official_grade_request(data, "x").prompt.find("off-by-one") != std::string::npos, "Temporal rule retained");
    data.evaluator.question_type = "single-session-preference";
    require(ltm::make_official_grade_request(data, "x").template_sha256 == "741ee3bcbea7ff5e8ed359acef61d2f8ded3de021bbcff6ee13de455f2e2aa9b", "Pinned preference template hash");
    require(ltm::make_official_grade_request(data, "x").prompt.find("Rubric:") != std::string::npos, "Preference rubric retained");
    data.evaluator.question_id = "q_abs";
    require(ltm::make_official_grade_request(data, "x").template_sha256 == "5c0b365a1e1d06db36377c735432b56e122ca3c428f89faf61d43a0d5a7e050b", "Pinned abstention template hash");
    require(ltm::make_official_grade_request(data, "x").prompt.find("unanswerable") != std::string::npos, "Abstention rule retained");
    data.evaluator.question_id = "q"; data.evaluator.question_type = "unknown";
    invalid([&] { (void)ltm::make_official_grade_request(data, "x"); });
    data.evaluator.question_type = "multi-session";
    require(ltm::make_official_grade_request(data, "x").template_sha256 == "fba020ba3d57982efdc9a937c1c01f897b789a608c7f88e60244121f6505e5bc", "Pinned general template hash");
    for (const auto& gold : {"1300", "-91", "0"}) {
        data.evaluator.answer_json = std::string("\"") + gold + "\"";
        const auto string_prompt = ltm::make_official_grade_request(data, "x").prompt;
        data.evaluator.answer_json = gold;
        require(ltm::make_official_grade_request(data, "x").prompt == string_prompt,
            "Integer gold uses decimal text in official prompt");
    }
    for (const auto& gold : {"true", "1.5", "null", "[1,2]"}) {
        data.evaluator.answer_json = gold;
        invalid([&] { (void)ltm::make_official_grade_request(data, "x"); });
    }
}
void verdicts() {
    require(ltm::parse_judge_verdict(" YES\n") == ltm::JudgeVerdict::yes, "Exact yes case-folded");
    require(ltm::parse_judge_verdict("no") == ltm::JudgeVerdict::no, "Exact no");
    for (const auto& text : {"yesterday", "yes or no", "yes.", "{\"label\":\"yes\"}", "", "not yes"})
        require(ltm::parse_judge_verdict(text) == ltm::JudgeVerdict::invalid, "Malformed verdict explicit");
}
void aggregate_checks() {
    const auto good = ltm::score_official_retrieval(sample(), {"s1", "s2", "s3"}, 2, ltm::RetrievalLevel::session);
    auto abs = good; abs.question_id = "a_abs"; abs.exclusion = ltm::RetrievalExclusion::abstention; abs.metrics.reset();
    const auto result = ltm::aggregate_official_retrieval({"q1", "a_abs", "missing"}, {good, abs});
    require(result.eligible == 1 && result.abstention == 1 && result.missing == 1, "Eligible denominator and missing explicit");
    close(*result.mean_recall_all, 1);
    invalid([&] { (void)ltm::aggregate_official_retrieval({"q1"}, {good, good}); });
    auto malformed = good; malformed.metrics->ndcg = std::nan("");
    invalid([&] { (void)ltm::aggregate_official_retrieval({"q1"}, {malformed}); });
}
void marker_semantics() {
    auto data = sample();
    for (auto& session : data.evaluator.sessions) {
        if (session.original_session_id == "answer_a") session.original_session_id = "background_a";
        if (session.original_session_id == "answer_b") session.original_session_id = "background_b";
    }
    for (auto& message : data.evaluator.messages) {
        if (message.original_session_id == "answer_a") message.original_session_id = "background_a";
        if (message.original_session_id == "answer_b") message.original_session_id = "background_b";
    }
    const auto result = ltm::score_official_retrieval(data, {"s1", "s2", "s3"}, 1, ltm::RetrievalLevel::session);
    close(result.metrics->recall_any, 0); close(result.metrics->recall_all, 1); close(result.metrics->ndcg, 0);
    require(ltm::aggregate_official_retrieval({"q1"}, {result}).eligible == 1, "Official empty-correct-set semantics preserved");
}
}
int main(int argc, char** argv) {
    if (argc == 2 && std::string(argv[1]) == "--parity-report") {
        nlohmann::json report = nlohmann::json::array();
        for (const bool second_target : {false, true}) {
            auto data = sample(); data.evaluator.messages[2].has_answer = second_target;
            for (int mode = 0; mode < 3; ++mode) {
                auto rank = mode == 0 ? std::vector<std::string>{"s1", "s2", "s3"} : std::vector<std::string>{"m1", "m3", "m4", "m5"};
                do {
                    for (std::size_t k = 1; k <= 5; ++k) {
                        const auto value = ltm::score_official_retrieval(data, rank, k, static_cast<ltm::RetrievalLevel>(mode));
                        report.push_back({{"mode", mode}, {"second_target", second_target}, {"rank", rank}, {"k", k},
                            {"any", value.metrics->recall_any}, {"all", value.metrics->recall_all}, {"ndcg", value.metrics->ndcg},
                            {"effective_k", value.metrics->effective_k}});
                    }
                } while (std::next_permutation(rank.begin(), rank.end()));
            }
        }
        std::cout << report.dump() << '\n'; return 0;
    }
    if (argc == 2 && std::string(argv[1]) == "--duplicate-report") {
        auto data = sample();
        data.evaluator.sessions[2].original_session_id = "answer_b";
        data.evaluator.messages[4].original_session_id = "answer_b";
        data.evaluator.messages[4].has_answer = true;
        nlohmann::json report = nlohmann::json::array();
        for (int mode = 0; mode < 3; ++mode) {
            std::vector<std::string> rank = mode == 0 ? std::vector<std::string>{"s1", "s2", "s3"} : std::vector<std::string>{"m1", "m3", "m4", "m5"};
            std::sort(rank.begin(), rank.end());
            do {
                for (std::size_t k = 1; k <= 3; ++k) {
                    const auto v = ltm::score_official_retrieval(data, rank, k, static_cast<ltm::RetrievalLevel>(mode));
                    report.push_back({{"mode", mode}, {"rank", rank}, {"k", k}, {"any", v.metrics->recall_any}, {"all", v.metrics->recall_all}, {"ndcg", v.metrics->ndcg}, {"effective_k", v.metrics->effective_k}});
                }
            } while (std::next_permutation(rank.begin(), rank.end()));
        }
        std::cout << report.dump() << '\n'; return 0;
    }
    if (argc == 2 && std::string(argv[1]) == "--prompt-report") {
        nlohmann::json report = nlohmann::json::array();
        for (const auto& type : {"single-session-user", "single-session-assistant", "multi-session", "temporal-reasoning", "knowledge-update", "single-session-preference"}) {
            auto data = sample(); data.evaluator.question_type = type;
            for (const bool abs : {false, true}) {
                data.evaluator.question_id = abs ? "q_abs" : "q";
                const auto value = ltm::make_official_grade_request(data, "frozen response");
                report.push_back({{"type", type}, {"abstention", abs}, {"prompt", value.prompt}, {"sha256", value.prompt_sha256}, {"template_sha256", value.template_sha256}});
            }
        }
        std::cout << report.dump() << '\n'; return 0;
    }
    if (argc != 1) return 2;
    int failed = 0;
    for (const auto& item : std::vector<std::pair<const char*, std::function<void()>>>{
        {"session metrics", sessions}, {"turn mapping", turn_mapping}, {"exclusions", exclusions},
        {"bad rankings", bad_rankings}, {"repeated source IDs", repeated_source_ids}, {"grade prompts", grade_prompts}, {"verdicts", verdicts},
        {"aggregates", aggregate_checks}, {"marker semantics", marker_semantics}}) {
        try { item.second(); std::cout << "PASS " << item.first << '\n'; }
        catch (const std::exception& e) { ++failed; std::cerr << "FAIL " << item.first << ": " << e.what() << '\n'; }
    }
    return failed ? 1 : 0;
}
