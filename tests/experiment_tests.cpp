#include "long_term_memory_benchmark/experiment.hpp"
#include "long_term_memory_benchmark/hash.hpp"
#include <functional>
#include <iostream>
#include <stdexcept>
#include <string>
#include <vector>

namespace {
void check(bool ok, const char* message) { if (!ok) throw std::runtime_error(message); }
template<class F> void rejects(F action) {
    bool rejected = false;
    try { action(); } catch (const std::invalid_argument&) { rejected = true; }
    check(rejected, "Invalid input must raise invalid_argument");
}
ltm::ExperimentManifest fixture() {
    ltm::ExperimentManifest m;
    m.source_revision = "source-1";
    m.dataset = {"public-fiction", "revision-1", ltm::sha256("fixture")};
    m.case_ids = {"q1", "q2", "q3", "q4"};
    m.method = "lexical"; m.method_revision = "v1";
    m.method_parameters = {{"top_k", "5"}, {"filter", "person"}};
    m.reader = {"fixture", "reader", "fixed-1"}; m.judge = {"fixture", "judge", "fixed-1"};
    m.prompt_sha256 = {{"answer", ltm::sha256("prompt")}};
    m.context_token_limit = 2048; m.output_token_limit = 128; m.concurrency = 2;
    m.run_parameters = {{"seed", "42"}, {"replicate", "0"}};
    return m;
}
void hash_vectors() {
    check(ltm::sha256("") == "e3b0c44298fc1c149afbf4c8996fb92427ae41e4649b934ca495991b7852b855", "SHA empty");
    check(ltm::sha256("abc") == "ba7816bf8f01cfea414140de5dae2223b00361a396177a9cb410ff61f20015ad", "SHA abc");
    check(ltm::sha256("abcdbcdecdefdefgefghfghighijhijkijkljklmklmnlmnomnopnopq") ==
        "248d6a61d20638b8e5c026930c3e6039a33ce45964ff2167f6ecedd419db06c1", "SHA multi-block");
    check(ltm::sha256(std::string(1000000, 'a')) ==
        "cdc76e5c9914fb9281a1c7e284d73e67f1809a48a497200e046d39ccc7112cd0", "SHA million a");
    check(ltm::sha256(std::string("a\0b", 3)) != ltm::sha256("a"), "SHA handles embedded null");
}
void manifest_roundtrip() {
    const auto m = fixture(); const auto text = ltm::manifest_to_json(m);
    check(!text.empty(), "Manifest serializes");
    const auto copy = ltm::manifest_from_json(text);
    check(ltm::manifest_to_json(copy) == text, "Manifest canonical roundtrip");
    check(copy.run_parameters == m.run_parameters && copy.method_parameters == m.method_parameters,
        "All declared parameters preserved");
    check(ltm::configuration_hash(copy) == ltm::sha256(text), "Configuration identity hashes canonical bytes");
}
void manifest_order() {
    const auto m = fixture(); auto text = ltm::manifest_to_json(m);
    const auto field = std::string("\"schema_version\":1,");
    const auto pos = text.find(field);
    check(pos != std::string::npos, "Find schema field");
    text.erase(pos, field.size()); text.insert(1, field);
    check(ltm::configuration_hash(ltm::manifest_from_json(text)) == ltm::configuration_hash(m),
        "Object key order does not change identity");
    auto changed = m; changed.run_parameters["seed"] = "43";
    check(ltm::configuration_hash(changed) != ltm::configuration_hash(m), "Run setting changes identity");
    changed = m; changed.reader.version = "fixed-2";
    check(ltm::configuration_hash(changed) != ltm::configuration_hash(m), "Reader revision changes identity");
    changed = m; changed.prompt_sha256["answer"] = ltm::sha256("changed");
    check(ltm::configuration_hash(changed) != ltm::configuration_hash(m), "Prompt changes identity");
}
void manifest_invalid() {
    auto m = fixture(); m.case_ids.push_back("q1"); rejects([&]{ ltm::manifest_to_json(m); });
    m = fixture(); m.concurrency = 0; rejects([&]{ ltm::manifest_to_json(m); });
    m = fixture(); m.dataset.sha256 = "short"; rejects([&]{ ltm::manifest_to_json(m); });
    m = fixture(); m.schema_version = 2; rejects([&]{ ltm::manifest_to_json(m); });
    rejects([]{ ltm::manifest_from_json("{}"); });
    rejects([]{ ltm::manifest_from_json("not-json"); });
    auto text = ltm::manifest_to_json(fixture());
    text.insert(1, "\"unknown\":true,"); rejects([&]{ ltm::manifest_from_json(text); });
    text = ltm::manifest_to_json(fixture());
    text.insert(1, "\"schema_version\":1,"); rejects([&]{ ltm::manifest_from_json(text); });
}
void every_field_identity() {
    const auto original=fixture();
    const std::vector<std::function<void(ltm::ExperimentManifest&)>> mutations = {
        [](auto& m){m.source_revision+="2";}, [](auto& m){m.dataset.name+="2";},
        [](auto& m){m.dataset.revision+="2";}, [](auto& m){m.dataset.sha256=ltm::sha256("other");},
        [](auto& m){std::swap(m.case_ids[0],m.case_ids[1]);}, [](auto& m){m.method+="2";},
        [](auto& m){m.method_revision+="2";}, [](auto& m){m.method_parameters["top_k"]="10";},
        [](auto& m){m.reader.provider+="2";}, [](auto& m){m.reader.id+="2";},
        [](auto& m){m.reader.version+="2";}, [](auto& m){m.judge.provider+="2";},
        [](auto& m){m.judge.id+="2";}, [](auto& m){m.judge.version+="2";},
        [](auto& m){m.prompt_sha256["answer"]=ltm::sha256("other");},
        [](auto& m){++m.context_token_limit;}, [](auto& m){++m.output_token_limit;},
        [](auto& m){++m.concurrency;}, [](auto& m){m.run_parameters["replicate"]="1";}};
    for(const auto& mutation:mutations) {
        auto changed=original; mutation(changed);
        check(ltm::configuration_hash(changed)!=ltm::configuration_hash(original), "Every declared experiment field changes identity");
    }
}
std::string replace_once(std::string text,const std::string& from,const std::string& to) {
    const auto pos=text.find(from); check(pos!=std::string::npos,"Test mutation locates field");
    text.replace(pos,from.size(),to); return text;
}
void strict_schema_types() {
    const auto text=ltm::manifest_to_json(fixture());
    for(const auto& value:std::vector<std::string>{"-1","2.5","true","\"2\"","4294967296"}) {
        rejects([&]{ltm::manifest_from_json(replace_once(text,"\"concurrency\":2","\"concurrency\":"+value));});
    }
    rejects([&]{ltm::manifest_from_json(replace_once(text,"\"dataset\":{","\"dataset\":{\"foreign\":0,"));});
    rejects([&]{ltm::manifest_from_json(replace_once(text,"\"method\":\"lexical\"","\"method\":null"));});
    auto m=fixture(); m.method=std::string(1,static_cast<char>(0xff));
    rejects([&]{ltm::manifest_to_json(m);});
}
void case_accounting() {
    std::vector<ltm::CaseRecord> records = {{"q1", ltm::CaseStatus::completed, "91", "", "r1", true},
        {"q2", ltm::CaseStatus::failed, "", "timeout", "r2", {}},
        {"q3", ltm::CaseStatus::uncertain, "", "submission unknown", "r3", {}}};
    const auto summary = ltm::summarize_cases(fixture(), records);
    check(summary.expected == 4 && summary.present == 3 && summary.missing == 1, "Full expected coverage");
    check(summary.completed == 1 && summary.failed == 1 && summary.uncertain == 1, "Lifecycle counts");
    check(summary.known_correct_fraction_of_expected == 0.25, "Missing failures remain in expected denominator");
    check(summary.graded_accuracy && *summary.graded_accuracy == 1.0, "Graded subset separately labeled");
    records.push_back(records[0]); rejects([&]{ ltm::summarize_cases(fixture(), records); });
    records.back().case_id = "foreign"; rejects([&]{ ltm::summarize_cases(fixture(), records); });
}
void result_roundtrip() {
    const auto m = fixture();
    const std::vector<ltm::CaseRecord> records = {{"q2", ltm::CaseStatus::queued, "", "", "", {}},
        {"q1", ltm::CaseStatus::completed, "quote\"\n91", "", "request", false}};
    const auto text = ltm::results_to_json(m, records);
    check(!text.empty(), "Results serialize");
    const auto copy = ltm::results_from_json(m, text);
    check(ltm::results_to_json(m, copy) == text, "Results stable roundtrip");
    check(copy[0].case_id == "q1" && copy[0].answer == "quote\"\n91", "Expected case order and escaped answer preserved");
    auto changed = m; changed.concurrency = 3;
    rejects([&]{ ltm::results_from_json(changed, text); });
    auto invalid = records; invalid[0].correct = true;
    rejects([&]{ ltm::results_to_json(m, invalid); });
    invalid = records; invalid[0].status = static_cast<ltm::CaseStatus>(77);
    rejects([&]{ ltm::results_to_json(m, invalid); });
}
void result_validation() {
    const auto m=fixture();
    const std::vector<ltm::CaseRecord> records={{"q1",ltm::CaseStatus::running,"","","r",{}},
        {"q2",ltm::CaseStatus::completed,"answer","","r2",{}},
        {"q3",ltm::CaseStatus::queued,"","","",{}}};
    const auto summary=ltm::summarize_cases(m,records);
    check(summary.queued==1 && summary.running==1 && summary.completed_ungraded==1 && !summary.graded_accuracy,
        "Pending and ungraded cases remain separate");
    const auto text=ltm::results_to_json(m,records);
    rejects([&]{ltm::results_from_json(m,replace_once(text,"\"running\"","\"invalid\""));});
    rejects([&]{ltm::results_from_json(m,replace_once(text,"\"missing\":1","\"missing\":0"));});
    rejects([&]{ltm::results_from_json(m,replace_once(text,"\"correct\":null","\"correct\":1"));});
    rejects([&]{ltm::results_from_json(m,replace_once(text,"\"schema_version\":1","\"schema_version\":2"));});
    auto changed=records; changed[0].status=ltm::CaseStatus::failed;
    rejects([&]{ltm::results_to_json(m,changed);});
    changed=records; changed[1].error="failed";
    rejects([&]{ltm::results_to_json(m,changed);});
    check(ltm::results_from_json(m,ltm::results_to_json(m,{})).empty(), "All-missing run still serializes coverage");
}
}
int main() {
    const std::vector<std::pair<const char*, std::function<void()>>> tests = {
        {"hash_vectors", hash_vectors}, {"manifest_roundtrip", manifest_roundtrip},
        {"manifest_order", manifest_order}, {"manifest_invalid", manifest_invalid},
        {"every_field_identity",every_field_identity},{"strict_schema_types",strict_schema_types},
        {"case_accounting", case_accounting}, {"result_roundtrip", result_roundtrip},
        {"result_validation",result_validation}};
    int failures = 0;
    for (const auto& test : tests) {
        try { test.second(); std::cout << "PASS " << test.first << '\n'; }
        catch (const std::exception& e) { ++failures; std::cerr << "FAIL " << test.first << ": " << e.what() << '\n'; }
    }
    return failures == 0 ? 0 : 1;
}
