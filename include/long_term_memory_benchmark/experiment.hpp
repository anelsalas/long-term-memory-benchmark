#pragma once
#include <cstddef>
#include <cstdint>
#include <map>
#include <optional>
#include <string>
#include <vector>

namespace ltm {
struct DatasetIdentity {
    std::string name;
    std::string revision;
    std::string sha256;
};
struct ModelIdentity {
    std::string provider;
    std::string id;
    std::string version;
};
struct ExperimentManifest {
    std::uint32_t schema_version = 1;
    std::string source_revision;
    DatasetIdentity dataset;
    std::vector<std::string> case_ids;
    std::string method;
    std::string method_revision;
    std::map<std::string, std::string> method_parameters;
    ModelIdentity reader;
    ModelIdentity judge;
    std::map<std::string, std::string> prompt_sha256;
    std::uint64_t context_token_limit = 0;
    std::uint64_t output_token_limit = 0;
    std::uint32_t concurrency = 1;
    std::map<std::string, std::string> run_parameters;
};

std::string manifest_to_json(const ExperimentManifest& manifest);
ExperimentManifest manifest_from_json(const std::string& text);
std::string configuration_hash(const ExperimentManifest& manifest);

enum class CaseStatus { queued, running, completed, failed, uncertain };
struct CaseRecord {
    std::string case_id;
    CaseStatus status = CaseStatus::queued;
    std::string answer;
    std::string error;
    std::string request_id;
    std::optional<bool> correct;
};
struct CaseSummary {
    std::size_t expected = 0, present = 0, missing = 0;
    std::size_t queued = 0, running = 0, completed = 0, failed = 0, uncertain = 0;
    std::size_t graded = 0, correct = 0, completed_ungraded = 0;
    double known_correct_fraction_of_expected = 0;
    std::optional<double> graded_accuracy;
};
CaseSummary summarize_cases(const ExperimentManifest& manifest, const std::vector<CaseRecord>& records);
std::string results_to_json(const ExperimentManifest& manifest, const std::vector<CaseRecord>& records);
std::vector<CaseRecord> results_from_json(const ExperimentManifest& manifest, const std::string& text);
} // namespace ltm
