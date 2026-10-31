#include "long_term_memory_benchmark/dashboard_plan.hpp"
#include <nlohmann/json.hpp>
#include <fstream>
#include <stdexcept>
#include <utility>

namespace ltm::dashboard {
namespace {
using Json = nlohmann::json;
Json model(const ModelIdentity& m) { return {{"id", m.id}, {"version", m.version}}; }
}

std::string compare_config(const std::string& plan_template, const Settings& settings) {
    auto plan = Json::parse(plan_template, nullptr, false);
    if (!plan.is_object() || !plan.contains("provider_policy") || !plan["provider_policy"].is_object())
        throw std::invalid_argument("Plan template must be a JSON object with a provider_policy object");
    plan["endpoint_url"] = settings.endpoint_url;
    plan["reader"] = model(settings.reader);
    plan["judge"] = model(settings.judge);
    auto& policy = plan["provider_policy"];
    policy["max_requests"] = std::to_string(settings.max_requests);
    policy["max_tokens"] = std::to_string(settings.max_tokens);
    policy["max_cost_nanos"] = std::to_string(settings.max_cost_nanos);
    plan["seed"] = std::to_string(settings.seed);
    plan["output_directory"] = "artifacts";
    return plan.dump(2);
}
PrepareRun compare_dry_run(std::string plan_template) {
    return [plan_template = std::move(plan_template)](const Settings& settings, const std::filesystem::path& folder) {
        const auto config = folder / "config.json";
        std::ofstream out(config, std::ios::binary | std::ios::trunc);
        out << compare_config(plan_template, settings);
        if (!out.flush()) throw std::runtime_error("Could not write " + config.string());
        return std::vector<std::string>{"--config", config.string(), "--dry-run"};
    };
}
}
