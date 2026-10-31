#include "dumas_memory_benchmark/memory.hpp"

#include <algorithm>
#include <set>
#include <stdexcept>
#include <utility>

namespace dmb {
namespace {
std::set<std::string> tokens(const std::string& text) {
    std::set<std::string> result;
    std::string token;
    for (unsigned char c : text) {
        if (c >= 'A' && c <= 'Z') c = static_cast<unsigned char>(c + ('a' - 'A'));
        if ((c >= 'a' && c <= 'z') || (c >= '0' && c <= '9')) {
            token += static_cast<char>(c);
        } else if (!token.empty()) {
            result.insert(std::move(token));
            token.clear();
        }
    }
    if (!token.empty()) result.insert(std::move(token));
    return result;
}

bool valid_timestamp(const std::string& value) {
    if (value.size() != 20 || value[4] != '-' || value[7] != '-' ||
        value[10] != 'T' || value[13] != ':' || value[16] != ':' || value[19] != 'Z') return false;
    for (std::size_t i = 0; i < value.size(); ++i) {
        if (i == 4 || i == 7 || i == 10 || i == 13 || i == 16 || i == 19) continue;
        if (value[i] < '0' || value[i] > '9') return false;
    }
    const auto year = std::stoi(value.substr(0, 4));
    const auto month = std::stoi(value.substr(5, 2));
    const auto day = std::stoi(value.substr(8, 2));
    if (year == 0 || month < 1 || month > 12 || day < 1) return false;
    const bool leap = year % 4 == 0 && (year % 100 != 0 || year % 400 == 0);
    const int days[] = {31, leap ? 29 : 28, 31, 30, 31, 30, 31, 31, 30, 31, 30, 31};
    return day <= days[month - 1] && std::stoi(value.substr(11, 2)) < 24 &&
        std::stoi(value.substr(14, 2)) < 60 && std::stoi(value.substr(17, 2)) < 60;
}
}

void LexicalMemory::store(Record record) {
    if (record.id.empty() || record.person.empty() || record.text.empty() || !valid_timestamp(record.timestamp))
        throw std::invalid_argument("Evidence requires id, person, text and a valid UTC timestamp");
    if (std::any_of(records_.begin(), records_.end(), [&](const Record& existing) { return existing.id == record.id; }))
        throw std::invalid_argument("Source id already exists");
    records_.push_back(std::move(record));
}

std::vector<Match> LexicalMemory::search(const Query& query) const {
    if (!query.as_of.empty() && !valid_timestamp(query.as_of))
        throw std::invalid_argument("Query as_of requires a valid UTC timestamp");
    const auto query_tokens = tokens(query.text);
    if (query.limit == 0 || query_tokens.empty()) return {};
    std::vector<Match> matches;
    for (const auto& record : records_) {
        if ((!query.person.empty() && record.person != query.person) ||
            (!query.as_of.empty() && record.timestamp > query.as_of)) continue;
        const auto record_tokens = tokens(record.text);
        std::size_t overlap = 0;
        for (const auto& token : query_tokens) overlap += record_tokens.count(token);
        if (overlap != 0) matches.push_back({record, overlap});
    }
    std::sort(matches.begin(), matches.end(), [](const Match& a, const Match& b) {
        if (a.score != b.score) return a.score > b.score;
        if (a.record.timestamp != b.record.timestamp) return a.record.timestamp > b.record.timestamp;
        return a.record.id < b.record.id;
    });
    if (matches.size() > query.limit) matches.resize(query.limit);
    return matches;
}
} // namespace dmb
