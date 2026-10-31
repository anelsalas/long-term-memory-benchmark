#include "long_term_memory_benchmark/dashboard_plan.hpp"
#include <nlohmann/json.hpp>
#include <chrono>
#include <filesystem>
#include <fstream>
#include <functional>
#include <iterator>
#include <iostream>
#include <stdexcept>
#include <string>
#include <utility>
#include <vector>

namespace {
using ltm::dashboard::Settings;
using Json = nlohmann::json;

void require(bool condition, const std::string& message) {
    if (!condition) throw std::runtime_error(message);
}
Settings settings() {
    Settings s;
    s.endpoint_url = "https://provider.example/v1/chat/completions";
    s.reader = {"example", "reader-model", "reader-2026-10"};
    s.judge = {"example", "judge-model", "judge-2026-09"};
    s.max_requests = 6; s.max_tokens = 18446744073709551615ull; s.max_cost_nanos = 250000000; s.seed = 42;
    return s;
}
const std::string plan_template = R"({"schema_version":1,"source_revision":"abc123",
  "endpoint_url":"https://template.example/v1","api_key_env":"LTM_PILOT_API_KEY",
  "reader":{"id":"t","version":"t"},"judge":{"id":"t","version":"t"},
  "provider_policy":{"max_requests":"0","max_tokens":"0","max_cost_nanos":"0","max_attempts":1},
  "seed":"1","top_k":3,"output_directory":"../runtime/pilot"})";

void fills_settings() {
    const auto plan = Json::parse(ltm::dashboard::compare_config(plan_template, settings()));
    require(plan.at("endpoint_url") == "https://provider.example/v1/chat/completions", "Endpoint comes from settings");
    require(plan.at("reader") == Json{{"id", "reader-model"}, {"version", "reader-2026-10"}}, "Reader id and version");
    require(plan.at("judge") == Json{{"id", "judge-model"}, {"version", "judge-2026-09"}}, "Judge id and version");
    const auto& policy = plan.at("provider_policy");
    require(policy.at("max_requests") == "6" && policy.at("max_tokens") == "18446744073709551615", "Request and token caps");
    require(policy.at("max_cost_nanos") == "250000000" && plan.at("seed") == "42", "Cost cap and seed");
    require(plan.at("output_directory") == "artifacts", "Artifacts stay in the run folder");
}
void keeps_template_fields() {
    const auto plan = Json::parse(ltm::dashboard::compare_config(plan_template, settings()));
    require(plan.at("source_revision") == "abc123" && plan.at("api_key_env") == "LTM_PILOT_API_KEY", "Reviewed fields kept");
    require(plan.at("top_k") == 3 && plan.at("provider_policy").at("max_attempts") == 1, "Other conditions kept");
    require(plan.size() == Json::parse(plan_template).size(), "No fields are added");
}
void rejects_bad_template() {
    for (const std::string bad : {"{", "[]", R"({"schema_version":1})", R"({"provider_policy":[]})"}) {
        bool refused = false;
        try { ltm::dashboard::compare_config(bad, settings()); } catch (const std::invalid_argument&) { refused = true; }
        require(refused, "Must reject template " + bad);
    }
}
void prepares_dry_run() {
    const auto folder = std::filesystem::temp_directory_path() / ("ltm-plan-" + std::to_string(
        std::chrono::steady_clock::now().time_since_epoch().count()));
    std::filesystem::create_directories(folder);
    const auto arguments = ltm::dashboard::compare_dry_run(plan_template)(settings(), folder);
    const auto config = folder / "config.json";
    std::ifstream in(config);
    const std::string written((std::istreambuf_iterator<char>(in)), {});
    in.close();
    std::error_code ignored; std::filesystem::remove_all(folder, ignored);
    require(arguments == std::vector<std::string>{"--config", config.string(), "--dry-run"}, "Runs a dry run of the config");
    require(Json::parse(written).at("seed") == "42", "The run's configuration is written to its folder");
}
}

int main(int argc, char** argv) {
    const std::vector<std::pair<std::string, std::function<void()>>> cases = {
        {"fills_settings", fills_settings}, {"keeps_template_fields", keeps_template_fields},
        {"rejects_bad_template", rejects_bad_template}, {"prepares_dry_run", prepares_dry_run}};
    if (argc != 2) return 2;
    for (const auto& item : cases) {
        if (item.first == argv[1]) {
            try { item.second(); std::cout << "PASS " << item.first << '\n'; return 0; }
            catch (const std::exception& e) { std::cerr << "FAIL " << item.first << ": " << e.what() << '\n'; return 1; }
        }
    }
    return 2;
}
