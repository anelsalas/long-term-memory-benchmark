#pragma once

#include "longmemeval.hpp"
#include <cstddef>
#include <cstdint>
#include <optional>
#include <string>
#include <vector>

namespace ltm {

struct FactEvent {
    std::string source_id;
    std::string person;
    std::string topic;
    std::string recorded_at;
    int score = 0;
    std::string corrects_source_id;
};

struct FactLedger {
    std::vector<FactEvent> events;
};

struct FactQuery {
    std::string person;
    std::string topic;
    std::string as_of;
};

struct FactAnswer {
    std::optional<int> score;
    std::vector<std::string> source_ids;
};

struct FactSourceMapping {
    std::string ledger_source_id;
    std::string candidate_source_id;
};

struct FreshFactCase {
    CandidateCase candidate;
    FactAnswer expected;
    std::vector<FactSourceMapping> source_mapping;
};

void validate_fact_ledger(const FactLedger& ledger);
FactLedger generate_development_ledger(std::uint64_t seed,
    std::size_t fact_families = 1, std::size_t distractors = 0);
FactLedger changed_fact_ledger(const FactLedger& original);
FactLedger remove_fact_history(const FactLedger& ledger,
    const std::string& person, const std::string& topic);
FactAnswer resolve_fact(const FactLedger& ledger, const FactQuery& query);
FreshFactCase render_fresh_fact_case(const FactLedger& ledger, const FactQuery& query);

} // namespace ltm
