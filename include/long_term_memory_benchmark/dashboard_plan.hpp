#pragma once
#include "long_term_memory_benchmark/dashboard_runs.hpp"
#include <string>

namespace ltm::dashboard {
// The ltm-compare configuration for one run: the reviewed plan template with the
// dashboard's endpoint, models, limits and seed filled in. Artifacts go to "artifacts"
// beside the written configuration.
void validate_compare_plan_template(const std::string& plan_template);
std::string compare_config(const std::string& plan_template, const Settings& settings);

// Prepares a dry run or explicit local execution from the saved settings.
PrepareRun compare_dry_run(std::string plan_template);
}
