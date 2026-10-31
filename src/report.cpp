#include "dumas_memory_benchmark/report.hpp"

#include <cmath>
#include <iomanip>
#include <locale>
#include <set>
#include <sstream>
#include <stdexcept>

namespace dmb {
namespace {
std::string json_string(const std::string& value) {
    const char* hex = "0123456789abcdef";
    std::string result = "\"";
    for (unsigned char c : value) {
        if (c == '"') result += "\\\"";
        else if (c == '\\') result += "\\\\";
        else if (c == '\n') result += "\\n";
        else if (c == '\r') result += "\\r";
        else if (c == '\t') result += "\\t";
        else if (c < 0x20) {
            result += "\\u00";
            result += hex[c >> 4];
            result += hex[c & 0xf];
        } else result += static_cast<char>(c);
    }
    return result + "\"";
}
}

std::string offline_report(const std::vector<RetrievalCheck>& checks) {
    std::ostringstream out;
    out.imbue(std::locale::classic());
    out << std::setprecision(17);
    out << "{\"schema_version\":1,\"run_kind\":\"offline_component\","
        << "\"fixture_kind\":\"public_synthetic_development\","
        << "\"method\":\"ascii_lexical_overlap\","
        << "\"source_recall_definition\":\"fraction_of_expected_source_ids\",\"model_calls\":0,"
        << "\"model_answer_accuracy\":null,\"model_tokens\":null,\"cases\":[";
    bool first_case = true;
    for (const auto& check : checks) {
        if (!std::isfinite(check.retrieval_ms) || check.retrieval_ms < 0)
            throw std::invalid_argument("Retrieval time must be finite and nonnegative");
        if (!first_case) out << ',';
        first_case = false;
        const std::set<std::string> expected(check.expected_sources.begin(), check.expected_sources.end());
        std::set<std::string> found;
        for (const auto& match : check.matches)
            if (expected.count(match.record.id)) found.insert(match.record.id);
        out << "{\"case_id\":" << json_string(check.case_id)
            << ",\"retrieval_ms\":" << check.retrieval_ms << ",\"source_recall\":";
        if (expected.empty()) out << "null";
        else out << static_cast<double>(found.size()) / static_cast<double>(expected.size());
        out << ",\"all_sources_recalled\":";
        if (expected.empty()) out << "null";
        else out << (found.size() == expected.size() ? "true" : "false");
        out << ",\"expected_source_count\":" << expected.size() << ",\"sources\":[";
        bool first_source = true;
        for (const auto& match : check.matches) {
            if (!first_source) out << ',';
            first_source = false;
            out << "{\"id\":" << json_string(match.record.id)
                << ",\"person\":" << json_string(match.record.person)
                << ",\"timestamp\":" << json_string(match.record.timestamp)
                << ",\"text\":" << json_string(match.record.text)
                << ",\"lexical_score\":" << match.score << '}';
        }
        out << "]}";
    }
    out << "]}";
    return out.str();
}
} // namespace dmb
