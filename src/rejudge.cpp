#include "long_term_memory_benchmark/rejudge.hpp"
#include "long_term_memory_benchmark/scoring.hpp"
#include <nlohmann/json.hpp>
#include <chrono>
#include <map>
#include <set>
#include <stdexcept>

namespace ltm::rejudge {
namespace {
using Json = nlohmann::json;
constexpr std::uint64_t response_byte_limit = 1024 * 1024;
constexpr std::uint64_t input_token_upper_bound = 1000000;

std::string text(const Json& object, const char* name) {
    if (!object.is_object() || !object.contains(name) || !object.at(name).is_string() ||
        object.at(name).get<std::string>().empty())
        throw std::invalid_argument(std::string("Missing text field: ") + name);
    return object.at(name).get<std::string>();
}
const char* verdict_name(JudgeVerdict verdict) {
    return verdict == JudgeVerdict::yes ? "yes" : verdict == JudgeVerdict::no ? "no" : "invalid";
}
}

std::vector<Item> parse_items(const std::string& json) {
    const auto root = Json::parse(json, nullptr, false);
    if (root.is_discarded() || !root.is_object() || !root.contains("items") || !root.at("items").is_array())
        throw std::invalid_argument("Expected {\"items\": [...]}");
    std::vector<Item> items;
    std::set<std::string> keys;
    for (const auto& entry : root.at("items")) {
        Item item{text(entry, "key"), text(entry, "prompt")};
        if (!keys.insert(item.key).second) throw std::invalid_argument("Duplicate key: " + item.key);
        items.push_back(std::move(item));
    }
    return items;
}

Verdict judge_item(Provider& judge, const ModelIdentity& model, const Item& item, std::uint64_t output_token_limit) {
    ProviderRequest request;
    request.dispatch_id = "rejudge:" + item.key;
    request.case_id = item.key;
    request.prompt = item.prompt;
    request.model = model;
    request.output_token_limit = output_token_limit;
    request.response_byte_limit = response_byte_limit;
    request.input_token_upper_bound = input_token_upper_bound;
    const auto start = std::chrono::steady_clock::now();
    const auto reply = judge.generate(request, {});
    Verdict verdict{item.key, "failed", reply.text, reply.error, reply.usage,
        std::chrono::duration<double, std::milli>(std::chrono::steady_clock::now() - start).count()};
    if (reply.status == ProviderStatus::success) verdict.verdict = verdict_name(parse_judge_verdict(reply.text));
    return verdict;
}

std::string results_json(const std::string& judge, const std::vector<Verdict>& verdicts) {
    Json list = Json::array();
    std::map<std::string, std::size_t> counts{{"yes", 0}, {"no", 0}, {"invalid", 0}, {"failed", 0}};
    for (const auto& verdict : verdicts) {
        ++counts[verdict.verdict];
        list.push_back({{"key", verdict.key}, {"verdict", verdict.verdict}, {"reply", verdict.reply},
            {"error", verdict.error}, {"wall_ms", verdict.wall_ms},
            {"usage", verdict.usage ? Json{{"input_tokens", verdict.usage->input_tokens},
                                           {"output_tokens", verdict.usage->output_tokens}} : Json(nullptr)}});
    }
    return Json{{"schema_version", 1}, {"judge", judge}, {"counts", counts}, {"verdicts", list}}.dump(2);
}

std::vector<Verdict> completed_verdicts(const std::string& results) {
    const auto root = Json::parse(results);
    std::vector<Verdict> verdicts;
    for (const auto& entry : root.at("verdicts")) {
        if (entry.at("verdict") == "failed") continue;
        Verdict verdict{entry.at("key"), entry.at("verdict"), entry.at("reply"), entry.at("error"), std::nullopt,
            entry.at("wall_ms").get<double>()};
        if (!entry.at("usage").is_null())
            verdict.usage = ProviderUsage{entry.at("usage").at("input_tokens"), entry.at("usage").at("output_tokens")};
        verdicts.push_back(std::move(verdict));
    }
    return verdicts;
}
} // namespace ltm::rejudge
