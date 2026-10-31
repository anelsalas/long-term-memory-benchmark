#pragma once
#include "long_term_memory_benchmark/experiment.hpp"
#include <cstdint>
#include <filesystem>
#include <optional>
#include <string>

namespace ltm::dashboard {
// Saved dashboard settings: which models to test and the hard limits for a run.
struct Settings {
    std::string endpoint_url;
    ModelIdentity reader;
    ModelIdentity judge;
    std::uint64_t max_requests = 0;
    std::uint64_t max_tokens = 0;
    std::uint64_t max_cost_nanos = 0;
    std::uint64_t seed = 0;
};

std::string settings_to_json(const Settings& settings);
Settings settings_from_json(const std::string& text);

// A missing file means nothing has been saved yet. Saving replaces the file whole.
std::optional<Settings> load_settings(const std::filesystem::path& file);
void save_settings(const std::filesystem::path& file, const Settings& settings);
}
