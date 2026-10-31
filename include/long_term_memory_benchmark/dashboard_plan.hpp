#pragma once
#include "long_term_memory_benchmark/dashboard_runs.hpp"
#include <string>

namespace ltm::dashboard {
// The ltm-compare configuration for one run: the reviewed plan template with the
// dashboard's endpoint, models, limits and seed filled in. Artifacts go to "artifacts"
// beside the written configuration.
std::string compare_config(const std::string& plan_template, const Settings& settings);

// Prepares each run as an ltm-compare dry run: zero provider calls.
PrepareRun compare_dry_run(std::string plan_template);
}
