#include "long_term_memory_benchmark/longmemeval.hpp"
#include "long_term_memory_benchmark/hash.hpp"
#include <nlohmann/json.hpp>
#include <algorithm>
#include <array>
#include <iomanip>
#include <locale>
#include <set>
#include <sstream>
#include <stdexcept>

namespace ltm {
namespace {
using Json = nlohmann::json;
[[noreturn]] void invalid(const char* what) { throw std::invalid_argument(what); }
const Json& field(const Json& value, const char* key) {
    if (!value.is_object() || !value.contains(key)) invalid("Required LongMemEval field missing");
    return value.at(key);
}
std::string text(const Json& value, bool allow_empty = false) {
    if (!value.is_string()) invalid("LongMemEval string field has incorrect type");
    auto result = value.get<std::string>();
    if (!allow_empty && result.empty()) invalid("LongMemEval identifier/text is empty");
    return result;
}
int digits(const std::string& value, std::size_t offset, std::size_t length) {
    int result = 0;
    for (std::size_t i = offset; i < offset + length; ++i) {
        if (value[i] < '0' || value[i] > '9') invalid("Invalid date digits");
        result = result * 10 + (value[i] - '0');
    }
    return result;
}
std::array<int, 6> date_key(const std::string& value) {
    std::array<int, 6> key{};
    if (value.size() == 22 && value[4] == '/' && value[7] == '/' &&
        value[10] == ' ' && value[11] == '(' && value[15] == ')' &&
        value[16] == ' ' && value[19] == ':') {
        const auto day = value.substr(12, 3);
        if (day != "Mon" && day != "Tue" && day != "Wed" && day != "Thu" &&
            day != "Fri" && day != "Sat" && day != "Sun") invalid("Invalid weekday label");
        key = {digits(value, 0, 4), digits(value, 5, 2), digits(value, 8, 2),
            digits(value, 17, 2), digits(value, 20, 2), 0};
    } else if (value.size() == 20 && value[4] == '-' && value[7] == '-' &&
        value[10] == 'T' && value[13] == ':' && value[16] == ':' && value[19] == 'Z') {
        key = {digits(value, 0, 4), digits(value, 5, 2), digits(value, 8, 2),
            digits(value, 11, 2), digits(value, 14, 2), digits(value, 17, 2)};
    } else invalid("Unsupported LongMemEval date format");
    const auto year = key[0], month = key[1], day = key[2];
    if (year < 1 || month < 1 || month > 12 || day < 1 ||
        key[3] > 23 || key[4] > 59 || key[5] > 59) invalid("Invalid calendar date");
    const bool leap = year % 4 == 0 && (year % 100 != 0 || year % 400 == 0);
    const int days[] = {31, leap ? 29 : 28, 31, 30, 31, 30, 31, 31, 30, 31, 30, 31};
    if (day > days[month - 1]) invalid("Invalid day of month");
    return key;
}
std::string opaque_id(const char* prefix, std::size_t index) {
    std::ostringstream out;
    out.imbue(std::locale::classic());
    out << prefix << std::setfill('0') << std::setw(6) << index + 1;
    return out.str();
}
bool valid_digest(const std::string& value) {
    return value.size() == 64 && std::all_of(value.begin(), value.end(), [](char c) {
        return (c >= '0' && c <= '9') || (c >= 'a' && c <= 'f');
    });
}
Json parse(std::string_view bytes) {
    std::vector<std::set<std::string>> keys;
    const auto callback = [&](int depth, Json::parse_event_t event, Json& value) {
        if (depth > 128) invalid("JSON nesting exceeds adapter limit");
        const auto level = static_cast<std::size_t>(depth);
        if (event == Json::parse_event_t::object_start) {
            if (keys.size() <= level + 1) keys.resize(level + 2);
            keys[level + 1].clear();
        } else if (event == Json::parse_event_t::key) {
            if (keys.size() <= level) keys.resize(level + 1);
            if (!keys[level].insert(value.get<std::string>()).second) invalid("Duplicate JSON object key");
        }
        return true;
    };
    try { return Json::parse(bytes.begin(), bytes.end(), callback); }
    catch (const Json::exception&) { invalid("Malformed LongMemEval JSON"); }
}
}

bool longmemeval_date_after(const std::string& left, const std::string& right) {
    return date_key(left) > date_key(right);
}

LoadedLongMemEvalCase load_longmemeval_case(std::string_view bytes,
    std::string_view question_id, const DatasetProvenance& provenance,
    HistoryTimePolicy time_policy) {
    if (time_policy != HistoryTimePolicy::require_not_after_question &&
        time_policy != HistoryTimePolicy::include_all) invalid("Invalid history time policy");
    constexpr std::size_t hard_limit = 512u * 1024u * 1024u;
    if (provenance.source_uri.empty() || provenance.revision.empty() ||
        !valid_digest(provenance.expected_sha256) || provenance.max_bytes == 0 ||
        provenance.max_bytes > hard_limit) invalid("Dataset provenance or byte limit invalid");
    if (bytes.empty() || bytes.size() > provenance.max_bytes) invalid("Dataset exceeds byte limit or is empty");
    const auto digest = sha256(bytes);
    if (digest != provenance.expected_sha256) invalid("Dataset SHA-256 mismatch");
    const auto data = parse(bytes);
    if (!data.is_array() || data.empty()) invalid("Dataset must be a nonempty JSON array");
    std::set<std::string> question_ids;
    const Json* selected = nullptr;
    std::size_t selected_index = 0;
    for (std::size_t i = 0; i < data.size(); ++i) {
        const auto id = text(field(data[i], "question_id"));
        if (!question_ids.insert(id).second) invalid("Duplicate question ID");
        if (id == question_id) { selected = &data[i]; selected_index = i; }
    }
    if (!selected) invalid("Selected question ID absent");
    const auto& entry = *selected;
    LoadedLongMemEvalCase result;
    result.provenance = provenance;
    result.actual_sha256 = digest;
    auto& candidate = result.candidate;
    auto& evaluator = result.evaluator;
    candidate.case_id = opaque_id("case-", selected_index);
    candidate.question = text(field(entry, "question"));
    candidate.question_date = text(field(entry, "question_date"));
    const auto question_date = date_key(candidate.question_date);
    evaluator.question_id = text(field(entry, "question_id"));
    evaluator.question_type = text(field(entry, "question_type"));
    const std::set<std::string> types = {"single-session-user", "single-session-assistant",
        "single-session-preference", "multi-session", "knowledge-update", "temporal-reasoning"};
    if (!types.count(evaluator.question_type)) invalid("Unsupported official question type");
    evaluator.abstention = evaluator.question_id.size() >= 4 &&
        evaluator.question_id.compare(evaluator.question_id.size() - 4, 4, "_abs") == 0;
    const auto& answer = field(entry, "answer");
    if (answer.is_null()) invalid("Gold answer is null");
    evaluator.answer_json = answer.dump();
    const auto& ids = field(entry, "haystack_session_ids");
    const auto& dates = field(entry, "haystack_dates");
    const auto& sessions = field(entry, "haystack_sessions");
    const auto& gold_sessions = field(entry, "answer_session_ids");
    if (!ids.is_array() || !dates.is_array() || !sessions.is_array() || !gold_sessions.is_array() ||
        ids.empty() || ids.size() != dates.size() || ids.size() != sessions.size())
        invalid("History session/date/ID arrays have invalid types or lengths");
    std::set<std::string> source_ids;
    for (std::size_t i = 0; i < sessions.size(); ++i) {
        const auto original_id = text(ids[i]);
        source_ids.insert(original_id);
        CandidateSession session{opaque_id("session-", i), text(dates[i]), {}};
        const auto date = date_key(session.date);
        if (time_policy == HistoryTimePolicy::require_not_after_question && date > question_date)
            invalid("History session occurs after question time");
        if (!sessions[i].is_array() || sessions[i].empty()) invalid("History session must contain messages");
        evaluator.sessions.push_back({session.source_id, original_id});
        for (std::size_t m = 0; m < sessions[i].size(); ++m) {
            const auto& message = sessions[i][m];
            const auto role = text(field(message, "role"));
            if (role != "user" && role != "assistant") invalid("Invalid history message role");
            const auto content = text(field(message, "content"), true);
            std::optional<bool> has_answer;
            if (message.contains("has_answer")) {
                if (!message.at("has_answer").is_boolean()) invalid("has_answer annotation must be Boolean");
                has_answer = message.at("has_answer").get<bool>();
            }
            auto annotations = message;
            annotations.erase("role"); annotations.erase("content");
            const auto message_id = session.source_id + "/" + opaque_id("message-", m);
            session.messages.push_back({message_id, role, content});
            evaluator.messages.push_back({message_id, original_id, m + 1, has_answer, annotations.dump()});
        }
        candidate.sessions.push_back(std::move(session));
    }
    std::stable_sort(candidate.sessions.begin(), candidate.sessions.end(),
        [](const CandidateSession& a, const CandidateSession& b) { return date_key(a.date) < date_key(b.date); });
    std::set<std::string> seen_gold;
    for (const auto& value : gold_sessions) {
        const auto id = text(value);
        if (!source_ids.count(id) || !seen_gold.insert(id).second) invalid("Invalid gold session reference");
        evaluator.answer_session_ids.push_back(id);
    }
    return result;
}

std::string candidate_input_json(const CandidateCase& candidate) {
    Json sessions = Json::array();
    for (const auto& session : candidate.sessions) {
        Json messages = Json::array();
        for (const auto& message : session.messages)
            messages.push_back({{"source_id", message.source_id}, {"role", message.role}, {"content", message.content}});
        sessions.push_back({{"source_id", session.source_id}, {"date", session.date}, {"messages", messages}});
    }
    return Json{{"case_id", candidate.case_id}, {"question", candidate.question},
        {"question_date", candidate.question_date}, {"sessions", sessions}}.dump();
}
} // namespace ltm
