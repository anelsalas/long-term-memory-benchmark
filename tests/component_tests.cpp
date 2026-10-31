#include <functional>
#include <iostream>
#include <limits>
#include <stdexcept>
#include <string>
#include <utility>
#include <vector>
#include "long_term_memory_benchmark/memory.hpp"
#include "long_term_memory_benchmark/report.hpp"

namespace {
void require(bool condition, const std::string& message) {
    if (!condition) throw std::runtime_error(message);
}
ltm::LexicalMemory school() {
    ltm::LexicalMemory memory;
    memory.store({"old", "maya", "2026-03-03T10:00:00Z", "Science project score 83"});
    memory.store({"new", "maya", "2026-03-12T10:00:00Z", "Science project score corrected 91"});
    memory.store({"other", "sam", "2026-03-13T10:00:00Z", "Science project score 72"});
    return memory;
}
void exact_source() {
    const auto result = school().search({"corrected", "maya", 5, ""});
    require(result.size() == 1 && result[0].record.id == "new", "Retrieve correction source");
}
void correction_order() {
    const auto result = school().search({"science project", "maya", 5, ""});
    require(result.size() == 2, "Preserve both original and corrected records");
    require(result[0].record.id == "new" && result[1].record.id == "old", "Newest breaks equal-score tie");
    require(result[1].record.text.find("83") != std::string::npos, "Original evidence remains intact");
}
void person_filter() {
    const auto result = school().search({"score", "sam", 5, ""});
    require(result.size() == 1 && result[0].record.id == "other", "Keep person-specific evidence separate");
}
void unknown_query() {
    require(school().search({"chemistry", "maya", 5, ""}).empty(), "Absent topic returns no records");
    require(school().search({"", "maya", 5, ""}).empty(), "Empty query returns no records");
}
void top_k() {
    require(school().search({"science", "", 1, ""}).size() == 1, "Limit retrieval count");
    require(school().search({"science", "", 0, ""}).empty(), "Zero limit returns no records");
    require(school().search({"science", "", 100, ""}).size() == 3, "Large limit bounded by matches");
}
void as_of() {
    const auto result = school().search({"score", "maya", 5, "2026-03-03T10:00:00Z"});
    require(result.size() == 1 && result[0].record.id == "old", "Future correction is absent before its date");
}
void duplicate_id() {
    auto memory = school();
    bool threw = false;
    try { memory.store({"old", "maya", "2026-03-20T10:00:00Z", "Replacement"}); }
    catch (const std::invalid_argument&) { threw = true; }
    require(threw, "Duplicate source IDs are rejected");
    require(memory.search({"83", "maya", 1, ""}).size() == 1, "Failed insert preserves evidence");
}
void empty_record() {
    for (const ltm::Record& record : std::vector<ltm::Record>{
        {"", "maya", "2026-03-03T10:00:00Z", "score"},
        {"id", "", "2026-03-03T10:00:00Z", "score"},
        {"id", "maya", "", "score"}, {"id", "maya", "2026-03-03T10:00:00Z", ""}}) {
        bool threw = false;
        try { ltm::LexicalMemory memory; memory.store(record); }
        catch (const std::invalid_argument&) { threw = true; }
        require(threw, "Incomplete evidence is rejected");
    }
}
void punctuation() {
    const auto result = school().search({"SCIENCE, PROJECT!", "maya", 5, ""});
    require(result.size() == 2 && result[0].score == 2, "Case and punctuation normalized");
}
void score_tie() {
    ltm::LexicalMemory memory;
    memory.store({"b", "maya", "2026-03-03T10:00:00Z", "science"});
    memory.store({"a", "maya", "2026-03-03T10:00:00Z", "science"});
    const auto result = memory.search({"science science", "maya", 5, ""});
    require(result.size() == 2 && result[0].record.id == "a", "Stable source-ID tie order");
    require(result[0].score == 1, "Repeated query terms do not inflate lexical overlap");
}
void report_escape() {
    const auto json = ltm::offline_report({{"case\"\n", {"x"}, {{{"x", "maya", "2026-03-03T10:00:00Z", "tab\tquote\""}, 1}}, 0.25}});
    require(json.find("case\\\"\\n") != std::string::npos, "Case identifiers JSON escaped");
    require(json.find("tab\\tquote\\\"") != std::string::npos, "Evidence text JSON escaped");
    require(json.find("\"model_answer_accuracy\":null") != std::string::npos, "Model accuracy explicitly unmeasured");
    require(json.find("\"source_recall\":1") != std::string::npos, "Expected evidence coverage calculated");
}
void invalid_timestamp() {
    for (const std::string stamp : {"2026-02-29T10:00:00Z", "2026-13-01T10:00:00Z",
         "2026-03-03T24:00:00Z", "2026-03-03", "2026-03-03T10:00:00+00:00"}) {
        bool threw = false;
        try { ltm::LexicalMemory memory; memory.store({"x", "maya", stamp, "score"}); }
        catch (const std::invalid_argument&) { threw = true; }
        require(threw, "Reject malformed or impossible source date");
    }
    ltm::LexicalMemory leap;
    leap.store({"leap", "maya", "2024-02-29T10:00:00Z", "score"});
    require(leap.search({"score", "maya", 1, "2024-02-29T10:00:00Z"}).size() == 1, "Accept valid leap date");
    bool threw = false;
    try { leap.search({"score", "maya", 1, "2024-02-30T10:00:00Z"}); }
    catch (const std::invalid_argument&) { threw = true; }
    require(threw, "Reject invalid historical cutoff");
}
void report_fraction() {
    const ltm::Match match{{"a", "maya", "2026-03-03T10:00:00Z", "score"}, 1};
    const auto partial = ltm::offline_report({{"partial", {"a", "a", "b"}, {match, match}, 0.0}});
    require(partial.find("\"source_recall\":0.5") != std::string::npos, "Count unique expected and found sources");
    require(partial.find("\"all_sources_recalled\":false") != std::string::npos, "Missing source visible");
    const auto absent = ltm::offline_report({{"no-gold", {}, {}, 0.0}});
    require(absent.find("\"source_recall\":null") != std::string::npos, "Undefined metric remains null");
    require(ltm::offline_report({}).find("\"cases\":[]") != std::string::npos, "Empty report stays valid");
}
void report_invalid_time() {
    for (const double elapsed : {-0.1, std::numeric_limits<double>::infinity(),
         std::numeric_limits<double>::quiet_NaN()}) {
        bool threw = false;
        try { (void)ltm::offline_report({{"bad", {}, {}, elapsed}}); }
        catch (const std::invalid_argument&) { threw = true; }
        require(threw, "Invalid timing cannot produce misleading JSON");
    }
}
void resource_delta() {
    ltm::ProcessResources before, after;
    before.cpu_total_ms = 12.5;
    after.cpu_total_ms = 18.0;
    before.resident_bytes = 100;
    after.resident_bytes = 80;
    after.process_lifetime_peak_resident_bytes = 200;
    const auto measurement = ltm::resource_interval("retrieval", before, after, 9.0);
    require(measurement.cpu_delta_ms && *measurement.cpu_delta_ms == 5.5, "CPU interval uses cumulative difference");
    require(measurement.before.resident_bytes == 100 && measurement.after.resident_bytes == 80,
        "Memory samples preserved separately from interval and lifetime peak");
    bool threw = false;
    try { (void)ltm::resource_interval("bad", {}, {}, -1); }
    catch (const std::invalid_argument&) { threw = true; }
    require(threw, "Reject invalid wall duration");
}
void resource_unavailable() {
    ltm::ProcessResources before, after;
    after.cpu_total_ms = 1;
    require(!ltm::resource_interval("missing", before, after, 1).cpu_delta_ms, "Missing CPU samples yield unavailable delta");
    before.cpu_total_ms = 2;
    require(!ltm::resource_interval("reset", before, after, 1).cpu_delta_ms, "Decreasing counter yields unavailable delta");
    before.cpu_total_ms = std::numeric_limits<double>::quiet_NaN();
    require(!ltm::resource_interval("invalid", before, after, 1).cpu_delta_ms, "Nonfinite sample yields unavailable delta");
}
void resource_json() {
    ltm::ProcessResources before, after;
    before.cpu_total_ms = std::numeric_limits<double>::quiet_NaN();
    after.resident_bytes = 64;
    const auto json = ltm::offline_report({}, {ltm::resource_interval("test", before, after, 1)});
    require(json.find("\"scope\":\"local_runner_process\"") != std::string::npos, "Resource scope explicit");
    require(json.find("\"cpu_delta_ms\":null") != std::string::npos, "Unavailable CPU serialized null");
    require(json.find("\"resident_bytes\":64") != std::string::npos, "Observed memory serialized");
    require(json.find("\"gpu_memory_bytes\":null") != std::string::npos, "GPU use unmeasured");
    require(json.find("\"cpu_total_ms\":null") != std::string::npos, "Invalid floating sample becomes null");
}
void resource_process() {
    const auto before = ltm::sample_process_resources();
    volatile std::uint64_t sink = 0;
    for (std::uint64_t i = 0; i < 100000; ++i) sink = sink + i;
    (void)sink;
    const auto after = ltm::sample_process_resources();
#if defined(_WIN32) || defined(__linux__)
    require(before.cpu_total_ms && after.cpu_total_ms, "Supported process CPU collection succeeds");
    require(after.resident_bytes && *after.resident_bytes > 0, "Supported process memory collection succeeds");
#endif
    if (before.cpu_total_ms && after.cpu_total_ms)
        require(*after.cpu_total_ms >= *before.cpu_total_ms, "CPU cumulative counters are monotonic");
    if (before.process_lifetime_peak_resident_bytes && after.process_lifetime_peak_resident_bytes)
        require(*after.process_lifetime_peak_resident_bytes >= *before.process_lifetime_peak_resident_bytes,
            "Lifetime resident high-water mark is monotonic");
}
}

int main(int argc, char** argv) {
    const std::vector<std::pair<std::string, std::function<void()>>> cases = {
        {"exact_source", exact_source}, {"correction_order", correction_order},
        {"person_filter", person_filter}, {"unknown_query", unknown_query},
        {"top_k", top_k}, {"as_of", as_of}, {"duplicate_id", duplicate_id},
        {"empty_record", empty_record}, {"punctuation", punctuation},
        {"score_tie", score_tie}, {"invalid_timestamp", invalid_timestamp},
        {"report_escape", report_escape}, {"report_fraction", report_fraction},
        {"report_invalid_time", report_invalid_time}, {"resource_delta", resource_delta},
        {"resource_unavailable", resource_unavailable}, {"resource_json", resource_json},
        {"resource_process", resource_process}};
    if (argc != 2) return 2;
    for (const auto& item : cases) {
        if (item.first == argv[1]) {
            try { item.second(); std::cout << "PASS " << item.first << '\n'; return 0; }
            catch (const std::exception& e) { std::cerr << "FAIL " << item.first << ": " << e.what() << '\n'; return 1; }
        }
    }
    return 2;
}
