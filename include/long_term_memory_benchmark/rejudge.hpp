#pragma once

#include "provider.hpp"
#include <optional>
#include <string>
#include <vector>

// Grades frozen answers again with another judge. Each item is an official judge
// prompt saved by ltm-compare, which already contains the question, the gold
// answer and the frozen answer; it is sent unchanged and read with P04's strict
// yes/no parser, so only the judge model differs.
namespace ltm::rejudge {
struct Item {
    std::string key;     // identifies the answer, e.g. "<question_id>|<condition>"
    std::string prompt;  // the saved official judge prompt
};
struct Verdict {
    std::string key;
    std::string verdict;  // yes, no, invalid (any other reply) or failed (no reply)
    std::string reply;
    std::string error;
    std::optional<ProviderUsage> usage;
    double wall_ms = 0;
};
std::vector<Item> parse_items(const std::string& json);
Verdict judge_item(Provider& judge, const ModelIdentity& model, const Item& item, std::uint64_t output_token_limit);
std::string results_json(const std::string& judge, const std::vector<Verdict>& verdicts);
// Verdicts from an earlier results file that need no retry (all but failed).
std::vector<Verdict> completed_verdicts(const std::string& results);
} // namespace ltm::rejudge
