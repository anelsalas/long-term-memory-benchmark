#include "long_term_memory_benchmark/fresh_facts.hpp"
#include <functional>
#include <iostream>
#include <stdexcept>

namespace {
void require(bool condition, const char* message) {
    if (!condition) throw std::runtime_error(message);
}
void rejects(const std::function<void()>& action) {
    bool rejected = false;
    try { action(); } catch (const std::invalid_argument&) { rejected = true; }
    require(rejected, "Invalid fixture input rejected");
}
ltm::FactLedger school() {
    return {{{"original", "Lina", "astronomy", "2026-03-03T10:00:00Z", 83, ""},
        {"other", "Robin", "astronomy", "2026-03-04T10:00:00Z", 72, ""},
        {"corrected", "Lina", "astronomy", "2026-03-12T10:00:00Z", 91, "original"}}};
}
void corrections() {
    const auto answer = ltm::resolve_fact(school(), {"Lina", "astronomy", "2026-03-13T10:00:00Z"});
    require(answer.score == 91 && answer.source_ids == std::vector<std::string>{"corrected"}, "Latest correction and supporting source");
}
void historical() {
    require(ltm::resolve_fact(school(), {"Lina", "astronomy", "2026-03-05T10:00:00Z"}).score == 83, "Historical state before correction");
    require(!ltm::resolve_fact(school(), {"Lina", "astronomy", "2026-03-01T10:00:00Z"}).score, "Future facts unavailable");
}
void identities() {
    require(ltm::resolve_fact(school(), {"Robin", "astronomy", "2026-03-13T10:00:00Z"}).score == 72, "Same topic for another person stays distinct");
    const auto absent = ltm::resolve_fact(school(), {"Lina", "chemistry", "2026-03-13T10:00:00Z"});
    require(!absent.score && absent.source_ids.empty(), "Missing topic has no answer or support");
}
void generation() {
    const auto a = ltm::generate_development_ledger(42, 3, 7);
    const auto b = ltm::generate_development_ledger(42, 3, 7);
    require(a.events.size() == 16, "Three fact families and declared distractors");
    const auto qa = ltm::render_fresh_fact_case(a, {"Lina-1", "astronomy", "2026-03-13T10:00:00Z"});
    const auto qb = ltm::render_fresh_fact_case(b, {"Lina-1", "astronomy", "2026-03-13T10:00:00Z"});
    require(ltm::candidate_input_json(qa.candidate) == ltm::candidate_input_json(qb.candidate), "Seeded development generation repeatable");
    ltm::validate_fact_ledger(a);
}
void changed_values() {
    const auto original = school();
    const auto changed = ltm::changed_fact_ledger(original);
    const auto q = ltm::FactQuery{"Lina", "astronomy", "2026-03-13T10:00:00Z"};
    const auto a = ltm::render_fresh_fact_case(original, q);
    const auto b = ltm::render_fresh_fact_case(changed, q);
    require(a.candidate.question == b.candidate.question && a.expected.score != b.expected.score, "Paired question fixed and expected fact changes");
    require(a.candidate.case_id == b.candidate.case_id, "Paired case identity depends on question rather than gold");
    require(original.events[0].score == 83 && original.events[2].score == 91, "Original retained");
    require(b.candidate.sessions.back().messages[0].content.find("100") != std::string::npos,
        "Correction narrative uses changed predecessor rather than old hard-coded number");
}
void evidence_removal() {
    const auto removed = ltm::remove_fact_history(school(), "Lina", "astronomy");
    require(!ltm::resolve_fact(removed, {"Lina", "astronomy", "2026-03-13T10:00:00Z"}).score, "Entire fact chain removal makes question unanswerable");
    require(removed.events.size() == 1 && removed.events[0].person == "Robin", "Unrelated evidence remains");
}
void candidate_boundary() {
    const auto result = ltm::render_fresh_fact_case(school(), {"Lina", "astronomy", "2026-03-05T10:00:00Z"});
    const auto input = ltm::candidate_input_json(result.candidate);
    require(result.candidate.sessions.size() == 2, "Only checkpoint history rendered");
    require(input.find("corrected") == std::string::npos && input.find("91") == std::string::npos, "Future correction absent");
    require(input.find("expected") == std::string::npos && input.find("seed") == std::string::npos, "Runtime contains history and question only");
    require(result.expected.source_ids == std::vector<std::string>{"session-000001/message-000001"}, "Expected source maps to opaque runtime ID");
    const auto other = ltm::render_fresh_fact_case(school(), {"Lina", "chemistry", "2026-03-05T10:00:00Z"});
    require(other.candidate.case_id != result.candidate.case_id, "Distinct questions have distinct case identity");
}
void correction_chain() {
    auto ledger = school();
    ledger.events.push_back({"second-correction", "Lina", "astronomy", "2026-03-14T10:00:00Z", 96, "corrected"});
    require(ltm::resolve_fact(ledger, {"Lina", "astronomy", "2026-03-15T10:00:00Z"}).score == 96, "Latest state across multiple corrections");
    require(ltm::resolve_fact(ledger, {"Lina", "astronomy", "2026-03-13T10:00:00Z"}).score == 91, "Historical state remains available in a longer chain");
    ledger.events.back().corrects_source_id = "original";
    rejects([&] { ltm::validate_fact_ledger(ledger); });
}
void invalid_ledgers() {
    auto bad = school(); bad.events[2].corrects_source_id = "missing";
    rejects([&] { ltm::validate_fact_ledger(bad); });
    bad = school(); bad.events[2].person = "Robin";
    rejects([&] { ltm::validate_fact_ledger(bad); });
    bad = school(); bad.events[2].score = 101;
    rejects([&] { ltm::validate_fact_ledger(bad); });
    bad = school(); bad.events[2].recorded_at = "2026-02-30T10:00:00Z";
    rejects([&] { ltm::validate_fact_ledger(bad); });
    bad = school(); bad.events[2].source_id = "original";
    rejects([&] { ltm::validate_fact_ledger(bad); });
    bad = school(); bad.events[2].corrects_source_id.clear();
    rejects([&] { ltm::validate_fact_ledger(bad); });
    rejects([] { (void)ltm::generate_development_ledger(1, 0, 0); });
    rejects([] { (void)ltm::generate_development_ledger(1, 129, 0); });
    rejects([] { (void)ltm::generate_development_ledger(1, 1, 10001); });
    rejects([] { (void)ltm::resolve_fact(school(), {"Lina", "astronomy", "2026-03-13T25:00:00Z"}); });
    const auto empty = ltm::remove_fact_history({{{"only", "Lina", "astronomy", "2026-03-03T10:00:00Z", 0, ""}}}, "Lina", "astronomy");
    require(empty.events.empty() && !ltm::resolve_fact(empty, {"Lina", "astronomy", "2026-03-04T10:00:00Z"}).score, "Empty history remains a valid unknown condition");
}
}
int main() {
    int failures = 0;
    for (const auto& test : std::vector<std::pair<const char*, std::function<void()>>>{
        {"corrections", corrections}, {"historical", historical}, {"identities", identities},
        {"generation", generation}, {"changed values", changed_values}, {"evidence removal", evidence_removal},
        {"candidate boundary", candidate_boundary}, {"correction chain", correction_chain}, {"invalid ledgers", invalid_ledgers}}) {
        try { test.second(); std::cout << "PASS " << test.first << '\n'; }
        catch (const std::exception& e) { ++failures; std::cerr << "FAIL " << test.first << ": " << e.what() << '\n'; }
    }
    return failures ? 1 : 0;
}
