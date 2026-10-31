#include "long_term_memory_benchmark/longmemeval.hpp"
#include "long_term_memory_benchmark/hash.hpp"
#include <nlohmann/json.hpp>
#include <fstream>
#include <functional>
#include <iostream>
#include <iterator>
#include <stdexcept>
#include <string>

namespace {
using Json = nlohmann::json;
void require(bool value, const char* message) { if (!value) throw std::runtime_error(message); }
std::string fixture() {
    std::ifstream in(LTM_FIXTURE_PATH, std::ios::binary);
    if (!in) throw std::runtime_error("Fixture read failed");
    return {std::istreambuf_iterator<char>(in), {}};
}
std::string file_bytes(const char* path) {
    std::ifstream in(path, std::ios::binary);
    if (!in) throw std::runtime_error("Input file read failed");
    return {std::istreambuf_iterator<char>(in), {}};
}
ltm::DatasetProvenance provenance(const std::string& bytes) {
    return {"fixture:original-fictional-school", "development-v1", ltm::sha256(bytes), 1024 * 1024};
}
ltm::LoadedLongMemEvalCase load(const std::string& bytes, const std::string& id = "fictional-school-01") {
    return ltm::load_longmemeval_case(bytes, id, provenance(bytes));
}
void rejects(const std::function<void()>& action) {
    bool failed = false;
    try { action(); } catch (const std::invalid_argument&) { failed = true; }
    require(failed, "Malformed dataset rejected with invalid_argument");
}
void schema() {
    const auto bytes = fixture();
    const auto result = load(bytes);
    require(result.actual_sha256 == ltm::sha256(bytes), "Exact input identity preserved");
    require(result.candidate.sessions.size() == 2, "Both sessions loaded");
    require(result.candidate.sessions[0].messages[1].role == "assistant", "Assistant role preserved");
    require(result.candidate.sessions[0].date == "2026/03/03 (Tue) 10:00", "Original date text preserved");
    require(result.evaluator.answer_json == "\"91\"", "Gold held by evaluator");
    require(result.evaluator.messages[0].has_answer == true, "Turn annotation retained in sidecar");
    require(result.evaluator.messages[0].original_message_number == 1, "Official one-based turn index retained");
    require(result.evaluator.sessions[0].original_session_id == "answer_school_01", "Session mapping retained");
}
void isolation() {
    const auto bytes = fixture();
    const auto clean = ltm::candidate_input_json(load(bytes).candidate);
    require(clean.find("answer_school") == std::string::npos, "Annotated original source IDs excluded");
    require(clean.find("has_answer") == std::string::npos, "Gold annotations excluded");
    require(clean.find("EVALUATOR_ONLY_MARKER") == std::string::npos, "Other annotations excluded");
    Json changed = Json::parse(bytes);
    changed[0]["answer"] = "DIFFERENT_GOLD";
    changed[0]["answer_session_ids"] = Json::array();
    changed[0]["question_type"] = "multi-session";
    changed[0]["haystack_sessions"][0][0]["has_answer"] = false;
    const auto serialized = changed.dump();
    require(clean == ltm::candidate_input_json(load(serialized).candidate), "Gold changes leave runtime input unchanged");
    changed[0]["question_id"] = "opaque-selection-abs_abs";
    changed[0]["haystack_session_ids"] = {"answer_SECRET_A", "answer_SECRET_B"};
    changed[0]["answer_session_ids"] = {"answer_SECRET_B"};
    require(clean == ltm::candidate_input_json(load(changed.dump(), "opaque-selection-abs_abs").candidate),
        "Original question/session identities and abstention annotations remain sidecar-only");
}
void identity() {
    const auto bytes = fixture(); auto p = provenance(bytes);
    p.expected_sha256 = std::string(64, '0');
    rejects([&] { (void)ltm::load_longmemeval_case(bytes, "fictional-school-01", p); });
    p = provenance(bytes); p.max_bytes = bytes.size() - 1;
    rejects([&] { (void)ltm::load_longmemeval_case(bytes, "fictional-school-01", p); });
    p = provenance(bytes); p.revision.clear();
    rejects([&] { (void)ltm::load_longmemeval_case(bytes, "fictional-school-01", p); });
    rejects([&] { (void)load(bytes + " ", "missing-case"); });
    require(load(bytes + " ").actual_sha256 != load(bytes).actual_sha256, "Whitespace changes byte identity");
}
void abstention() {
    const auto result = load(fixture(), "fictional-school-02_abs");
    require(result.evaluator.abstention, "Official abstention suffix recognized");
    require(result.evaluator.answer_session_ids.empty(), "Abstention can have no gold sessions");
}
void malformed() {
    const auto base = Json::parse(fixture());
    const auto reject_edit = [&](const std::function<void(Json&)>& edit) {
        auto value = base; edit(value); const auto text = value.dump();
        rejects([&] { (void)load(text); });
    };
    reject_edit([](Json& j) { j[0]["haystack_dates"].erase(0); });
    reject_edit([](Json& j) { j[0]["haystack_session_ids"][1] = j[0]["haystack_session_ids"][0]; });
    reject_edit([](Json& j) { j[1]["question_id"] = j[0]["question_id"]; });
    reject_edit([](Json& j) { j[0]["haystack_dates"][0] = "2026/02/30 (Mon) 09:00"; });
    reject_edit([](Json& j) { j[0]["haystack_dates"][0] = "2026/03/14 (Sat) 09:00"; });
    reject_edit([](Json& j) { j[0]["haystack_sessions"][0][0]["role"] = "tool"; });
    reject_edit([](Json& j) { j[0]["haystack_sessions"][0][0].erase("content"); });
    reject_edit([](Json& j) { j[0]["question"] = 7; });
    reject_edit([](Json& j) { j[0]["haystack_sessions"][0][0]["has_answer"] = "yes"; });
    reject_edit([](Json& j) { j[0]["answer_session_ids"] = {"unknown"}; });
    reject_edit([](Json& j) { j[0]["question_type"] = "unknown"; });
    reject_edit([](Json& j) { j[0]["haystack_sessions"][0] = Json::array(); });
    reject_edit([](Json& j) { j[0]["answer_session_ids"] = {"answer_school_01", "answer_school_01"}; });
    reject_edit([](Json& j) { j[0]["question_date"] = "2026/03/13 (Fri) 24:00"; });
    rejects([&] { (void)load("{bad-json"); });
    rejects([&] { (void)load("{}"); });
    rejects([&] { (void)load("[{\"question_id\":\"a\",\"question_id\":\"b\"}]"); });
}
void chronology() {
    auto data = Json::parse(fixture());
    data[0]["haystack_dates"] = {"2026/03/12 (Thu) 10:00", "2026/03/03 (Tue) 10:00"};
    const auto text = data.dump(); rejects([&] { (void)load(text); });
}
void iso_dates() {
    auto data = Json::parse(fixture());
    data[0]["question_date"] = "2026-03-13T09:00:00Z";
    data[0]["haystack_dates"] = {"2026-03-03T10:00:00Z", "2026-03-12T10:00:00Z"};
    const auto result = load(data.dump());
    require(result.candidate.question_date == "2026-03-13T09:00:00Z", "ISO date preserved exactly");
}
}
int main(int argc, char** argv) {
    if (argc == 4 && std::string(argv[1]) == "--validate-file") {
        try {
            const auto bytes = file_bytes(argv[2]);
            const auto metadata = Json::parse(file_bytes(argv[3]));
            ltm::DatasetProvenance p{metadata.at("source_uri").get<std::string>(),
                metadata.at("revision").get<std::string>(), metadata.at("sha256").get<std::string>()};
            const auto result = ltm::load_longmemeval_case(bytes, metadata.at("case_id").get<std::string>(), p);
            const auto sanitized = Json::parse(ltm::candidate_input_json(result.candidate));
            require(sanitized.at("sessions").size() == result.evaluator.sessions.size(), "Source mapping complete");
            std::cout << "PASS external schema validation: bytes=" << bytes.size()
                << " sessions=" << result.candidate.sessions.size()
                << " messages=" << result.evaluator.messages.size() << '\n';
            return 0;
        } catch (const std::exception& e) { std::cerr << "FAIL external schema: " << e.what() << '\n'; return 1; }
    }
    if (argc != 1) return 2;
    int failed = 0;
    for (const auto& item : std::vector<std::pair<const char*, std::function<void()>>>{
        {"schema", schema}, {"gold isolation", isolation}, {"identity", identity},
        {"abstention", abstention}, {"malformed", malformed}, {"chronology", chronology},
        {"ISO dates", iso_dates}}) {
        try { item.second(); std::cout << "PASS " << item.first << '\n'; }
        catch (const std::exception& e) { ++failed; std::cerr << "FAIL " << item.first << ": " << e.what() << '\n'; }
    }
    return failed ? 1 : 0;
}
