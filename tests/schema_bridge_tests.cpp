#include "long_term_memory_benchmark/experiment.hpp"
#include "long_term_memory_benchmark/hash.hpp"
#include "long_term_memory_benchmark/longmemeval.hpp"
#include <fstream>
#include <iostream>
#include <iterator>
#include <stdexcept>

int main() {
    try {
        std::ifstream input(LTM_FIXTURE_PATH, std::ios::binary);
        if (!input) throw std::runtime_error("Fixture unavailable");
        const std::string bytes((std::istreambuf_iterator<char>(input)), {});
        const ltm::DatasetProvenance provenance{"fixture:original-fictional-development", "1", ltm::sha256(bytes)};
        const auto loaded = ltm::load_longmemeval_case(bytes, "fictional-school-01", provenance);
        ltm::ExperimentManifest manifest;
        manifest.source_revision = "schema-bridge-test";
        manifest.dataset = {"fictional-development", provenance.revision, loaded.actual_sha256};
        manifest.case_ids = {loaded.candidate.case_id};
        manifest.method = "fixture-input";
        manifest.method_revision = "1";
        manifest.reader = {"fixture", "uninvoked", "1"};
        manifest.judge = {"fixture", "uninvoked", "1"};
        const auto runtime = ltm::candidate_input_json(loaded.candidate);
        manifest.prompt_sha256 = {{"candidate-input", ltm::sha256(runtime)}};
        manifest.context_token_limit = 1000;
        manifest.output_token_limit = 100;
        if (runtime.find("\"has_answer\"") != std::string::npos ||
            runtime.find("\"answer_session_ids\"") != std::string::npos)
            throw std::runtime_error("Evaluation annotations crossed input boundary");
        const std::vector<ltm::CaseRecord> queued{{loaded.candidate.case_id}};
        const auto encoded = ltm::results_to_json(manifest, queued);
        const auto decoded = ltm::results_from_json(manifest, encoded);
        const auto summary = ltm::summarize_cases(manifest, decoded);
        if (summary.expected != 1 || summary.queued != 1 || summary.graded != 0)
            throw std::runtime_error("Dataset case accounting changed across serialization");
        ++manifest.output_token_limit;
        bool rejected = false;
        try { (void)ltm::results_from_json(manifest, encoded); }
        catch (const std::invalid_argument&) { rejected = true; }
        if (!rejected) throw std::runtime_error("Changed configuration accepted prior results");
        std::cout << "PASS dataset-to-manifest bridge and result identity\n";
        return 0;
    } catch (const std::exception& error) {
        std::cerr << "FAIL schema bridge: " << error.what() << '\n';
        return 1;
    }
}
