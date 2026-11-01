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

void validate_compare_plan_template(const std::string& plan_template) {
    const auto plan = Json::parse(plan_template, nullptr, false);
    if (!plan.is_object() || !plan.contains("provider_policy") || !plan["provider_policy"].is_object())
        throw std::invalid_argument("Plan template must be a JSON object with a provider_policy object");
}

std::string compare_config(const std::string& plan_template, const Settings& settings) {
    (void)settings_from_json(settings_to_json(settings));
    validate_compare_plan_template(plan_template);
    auto plan = Json::parse(plan_template, nullptr, false);
    if (!plan.is_object() || !plan.contains("provider_policy") || !plan["provider_policy"].is_object())
        throw std::invalid_argument("Plan template must be a JSON object with a provider_policy object");
    plan["endpoint_url"] = settings.endpoint_url;
    plan["reader"] = model(settings.reader);
    plan["judge"] = model(settings.judge);
    auto& policy = plan["provider_policy"];
    if (settings.run_mode == "local") {
        if (plan.value("prompt_profile", std::string()) != "compact_local_smoke" ||
            !plan.value("tokenizer_bound_verified", false))
            throw std::invalid_argument("Local runs require a reviewed compact local plan with verified token bounds");
        plan["execution_mode"] = "local";
        plan.erase("api_key_env");
        for (const auto* field : {"input_token_price_nanos", "output_token_price_nanos", "request_price_nanos"})
            policy[field] = "0";
    } else {
        // A local template can also be inspected without dispatch.
        if (plan.value("execution_mode", std::string("https")) == "local") {
            plan["execution_mode"] = "https";
            plan["prompt_profile"] = "standard";
            plan["api_key_env"] = "LTM_DASHBOARD_DRY_RUN_UNUSED";
        }
    }
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
        return std::vector<std::string>{"--config", config.string(), settings.run_mode == "local" ? "--execute" : "--dry-run"};
    };
}
}
