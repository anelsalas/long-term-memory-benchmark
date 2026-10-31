#include "long_term_memory_benchmark/fresh_facts.hpp"
#include "long_term_memory_benchmark/hash.hpp"
#include <algorithm>
#include <iomanip>
#include <locale>
#include <map>
#include <set>
#include <sstream>
#include <stdexcept>

namespace ltm {
namespace {

void validate_label(const std::string& label) {
    if (label.empty() || label.size() > 128)
        throw std::invalid_argument("A fictional label requires 1 to 128 characters");
    bool has_identifier = false;
    for (const unsigned char c : label) {
        const bool identifier = (c >= 'a' && c <= 'z') || (c >= 'A' && c <= 'Z') ||
            (c >= '0' && c <= '9');
        has_identifier = has_identifier || identifier;
        if (!identifier && c != ' ' && c != '-' && c != '_')
            throw std::invalid_argument("Fictional labels use ASCII letters, digits, spaces, hyphens and underscores");
    }
    if (!has_identifier) throw std::invalid_argument("Fictional label requires an identifier");
}

void validate_date(const std::string& value) {
    if (value.size() != 20 || value[4] != '-' || value[7] != '-' ||
        value[10] != 'T' || value[13] != ':' || value[16] != ':' || value[19] != 'Z')
        throw std::invalid_argument("Fictional event date requires UTC ISO 8601 seconds");
    for (std::size_t i = 0; i < value.size(); ++i) {
        if (i == 4 || i == 7 || i == 10 || i == 13 || i == 16 || i == 19) continue;
        if (value[i] < '0' || value[i] > '9') throw std::invalid_argument("Invalid date digits");
    }
    const int year = std::stoi(value.substr(0, 4));
    const int month = std::stoi(value.substr(5, 2));
    const int day = std::stoi(value.substr(8, 2));
    if (year < 1 || month < 1 || month > 12 || day < 1 ||
        std::stoi(value.substr(11, 2)) > 23 || std::stoi(value.substr(14, 2)) > 59 ||
        std::stoi(value.substr(17, 2)) > 59) throw std::invalid_argument("Invalid calendar/time value");
    const bool leap = year % 4 == 0 && (year % 100 != 0 || year % 400 == 0);
    const int days[] = {31, leap ? 29 : 28, 31, 30, 31, 30, 31, 31, 30, 31, 30, 31};
    if (day > days[month - 1]) throw std::invalid_argument("Invalid day of month");
}

void validate_query(const FactQuery& query) {
    validate_label(query.person);
    validate_label(query.topic);
    validate_date(query.as_of);
}

std::string numbered_id(const char* prefix, std::size_t index) {
    std::ostringstream out;
    out.imbue(std::locale::classic());
    out << prefix << std::setfill('0') << std::setw(6) << index + 1;
    return out.str();
}

std::uint64_t development_next(std::uint64_t& state) {
    // Fixed SplitMix64 steps make public development seeds reproducible.
    state += UINT64_C(0x9e3779b97f4a7c15);
    auto value = state;
    value = (value ^ (value >> 30)) * UINT64_C(0xbf58476d1ce4e5b9);
    value = (value ^ (value >> 27)) * UINT64_C(0x94d049bb133111eb);
    return value ^ (value >> 31);
}

FactAnswer resolve_validated(const FactLedger& ledger, const FactQuery& query) {
    FactAnswer answer;
    for (const auto& event : ledger.events) {
        if (event.recorded_at > query.as_of) break;
        if (event.person == query.person && event.topic == query.topic) {
            answer.score = event.score;
            answer.source_ids = {event.source_id};
        }
    }
    return answer;
}

} // namespace

void validate_fact_ledger(const FactLedger& ledger) {
    if (ledger.events.size() > 20000) throw std::invalid_argument("Ledger exceeds 20000-event limit");
    std::map<std::string, const FactEvent*> sources;
    std::map<std::pair<std::string, std::string>, const FactEvent*> latest;
    std::string previous_date;
    for (const auto& event : ledger.events) {
        validate_label(event.source_id);
        validate_label(event.person);
        validate_label(event.topic);
        validate_date(event.recorded_at);
        if (event.score < 0 || event.score > 100) throw std::invalid_argument("Score requires range 0 to 100");
        if (!previous_date.empty() && event.recorded_at < previous_date)
            throw std::invalid_argument("Ledger events must be chronological");
        if (sources.count(event.source_id)) throw std::invalid_argument("Duplicate ledger source ID");
        const auto key = std::make_pair(event.person, event.topic);
        const auto earlier = latest.find(key);
        if (event.corrects_source_id.empty()) {
            if (earlier != latest.end()) throw std::invalid_argument("Updated score requires an explicit correction reference");
        } else {
            const auto corrected = sources.find(event.corrects_source_id);
            if (corrected == sources.end() || earlier == latest.end() ||
                earlier->second != corrected->second ||
                corrected->second->recorded_at >= event.recorded_at)
                throw std::invalid_argument("Correction must reference the previous state of the same person/topic");
        }
        sources.emplace(event.source_id, &event);
        latest[key] = &event;
        previous_date = event.recorded_at;
    }
}

FactLedger generate_development_ledger(std::uint64_t seed,
    std::size_t fact_families, std::size_t distractors) {
    if (fact_families < 1 || fact_families > 128 || distractors > 10000)
        throw std::invalid_argument("Development generation requires 1 to 128 families and at most 10000 distractors");
    FactLedger ledger;
    std::vector<int> scores;
    for (std::size_t i = 0; i < fact_families; ++i) {
        const int score = static_cast<int>(development_next(seed) % 101);
        scores.push_back(score);
        ledger.events.push_back({numbered_id("ledger-", ledger.events.size()),
            "Lina-" + std::to_string(i + 1), "astronomy", "2026-03-03T10:00:00Z", score, ""});
    }
    for (std::size_t i = 0; i < fact_families; ++i) {
        ledger.events.push_back({numbered_id("ledger-", ledger.events.size()),
            "Robin-" + std::to_string(i + 1), "astronomy", "2026-03-04T10:00:00Z", (scores[i] + 37) % 101, ""});
    }
    for (std::size_t i = 0; i < fact_families; ++i) {
        const auto prior_id = ledger.events[i].source_id;
        ledger.events.push_back({numbered_id("ledger-", ledger.events.size()),
            "Lina-" + std::to_string(i + 1), "astronomy", "2026-03-12T10:00:00Z", (scores[i] + 9) % 101, prior_id});
    }
    for (std::size_t i = 0; i < distractors; ++i) {
        ledger.events.push_back({numbered_id("ledger-", ledger.events.size()),
            "Visitor-" + std::to_string(i + 1), "music", "2026-03-13T09:00:00Z",
            static_cast<int>(development_next(seed) % 101), ""});
    }
    validate_fact_ledger(ledger);
    return ledger;
}

FactLedger changed_fact_ledger(const FactLedger& original) {
    validate_fact_ledger(original);
    auto changed = original;
    for (auto& event : changed.events) event.score = (event.score + 17) % 101;
    validate_fact_ledger(changed);
    return changed;
}

FactLedger remove_fact_history(const FactLedger& ledger,
    const std::string& person, const std::string& topic) {
    validate_fact_ledger(ledger);
    validate_label(person);
    validate_label(topic);
    FactLedger removed;
    for (const auto& event : ledger.events)
        if (event.person != person || event.topic != topic) removed.events.push_back(event);
    validate_fact_ledger(removed);
    return removed;
}

FactAnswer resolve_fact(const FactLedger& ledger, const FactQuery& query) {
    validate_fact_ledger(ledger);
    validate_query(query);
    return resolve_validated(ledger, query);
}

FreshFactCase render_fresh_fact_case(const FactLedger& ledger, const FactQuery& query) {
    validate_fact_ledger(ledger);
    validate_query(query);
    FreshFactCase result;
    result.expected = resolve_validated(ledger, query);
    const auto query_identity = std::to_string(query.person.size()) + ":" + query.person +
        std::to_string(query.topic.size()) + ":" + query.topic + query.as_of;
    result.candidate.case_id = "fresh-" + sha256(query_identity);
    result.candidate.question_date = query.as_of;
    result.candidate.question = "As of " + query.as_of + ", what score was recorded for " +
        query.person + " in " + query.topic + "?";
    std::map<std::string, const FactEvent*> previous;
    for (std::size_t i = 0; i < ledger.events.size(); ++i) {
        const auto& event = ledger.events[i];
        if (event.recorded_at > query.as_of) break;
        const auto session_id = numbered_id("session-", i);
        const auto message_id = session_id + "/message-000001";
        std::string content;
        if (event.corrects_source_id.empty()) {
            content = "I recorded " + event.person + "'s " + event.topic + " score as " + std::to_string(event.score) + ".";
        } else {
            const auto& prior = *previous.at(event.corrects_source_id);
            content = event.person + "'s previous " + event.topic + " score of " + std::to_string(prior.score) +
                " has been corrected to " + std::to_string(event.score) + ".";
        }
        result.candidate.sessions.push_back({session_id, event.recorded_at, {{message_id, "user", content}}});
        result.source_mapping.push_back({event.source_id, message_id});
        if (!result.expected.source_ids.empty() && result.expected.source_ids[0] == event.source_id)
            result.expected.source_ids[0] = message_id;
        previous.emplace(event.source_id, &event);
    }
    return result;
}

} // namespace ltm
