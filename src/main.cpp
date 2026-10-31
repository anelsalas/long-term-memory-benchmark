#include <chrono>
#include <iostream>
#include <string>
#include <utility>
#include "long_term_memory_benchmark/memory.hpp"
#include "long_term_memory_benchmark/report.hpp"

int main(int argc, char** argv) {
    if (argc == 2 && std::string(argv[1]) == "--help") {
        std::cout << "Usage: long-term-memory-benchmark --offline-demo\n"
                  << "Runs public fictional lexical-retrieval component checks; prints JSON.\n";
        return 0;
    }
    if (argc != 2 || std::string(argv[1]) != "--offline-demo") {
        std::cerr << "Usage: long-term-memory-benchmark --offline-demo\n";
        return 2;
    }
    const auto run_before = ltm::sample_process_resources();
    const auto run_start = std::chrono::steady_clock::now();
    const auto ingestion_before = ltm::sample_process_resources();
    const auto ingestion_start = std::chrono::steady_clock::now();
    ltm::LexicalMemory memory;
    memory.store({"school-01", "maya", "2026-03-03T10:00:00Z", "Science project score 83."});
    memory.store({"school-02", "maya", "2026-03-12T10:00:00Z", "Science project score corrected to 91."});
    memory.store({"school-03", "sam", "2026-03-13T10:00:00Z", "Science project score 72."});
    const auto ingestion_wall = std::chrono::duration<double, std::milli>(
        std::chrono::steady_clock::now() - ingestion_start).count();
    const auto ingestion_after = ltm::sample_process_resources();
    const auto retrieval_before = ltm::sample_process_resources();
    const auto start = std::chrono::steady_clock::now();
    auto matches = memory.search({"science project score", "maya", 2, ""});
    const auto elapsed = std::chrono::duration<double, std::milli>(
        std::chrono::steady_clock::now() - start).count();
    const auto retrieval_after = ltm::sample_process_resources();
    const auto run_wall = std::chrono::duration<double, std::milli>(
        std::chrono::steady_clock::now() - run_start).count();
    const auto run_after = ltm::sample_process_resources();
    std::cout << ltm::offline_report({{"school-correction-sources", {"school-01", "school-02"},
        std::move(matches), elapsed}}, {
            ltm::resource_interval("run_work", run_before, run_after, run_wall),
            ltm::resource_interval("ingestion", ingestion_before, ingestion_after, ingestion_wall),
            ltm::resource_interval("retrieval", retrieval_before, retrieval_after, elapsed)}) << '\n';
    return 0;
}
