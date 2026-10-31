#include <chrono>
#include <iostream>
#include <string>
#include <utility>
#include "dumas_memory_benchmark/memory.hpp"
#include "dumas_memory_benchmark/report.hpp"

int main(int argc, char** argv) {
    if (argc == 2 && std::string(argv[1]) == "--help") {
        std::cout << "Usage: dumas-memory-benchmark --offline-demo\n"
                  << "Runs public fictional lexical-retrieval component checks; prints JSON.\n";
        return 0;
    }
    if (argc != 2 || std::string(argv[1]) != "--offline-demo") {
        std::cerr << "Usage: dumas-memory-benchmark --offline-demo\n";
        return 2;
    }
    dmb::LexicalMemory memory;
    memory.store({"school-01", "maya", "2026-03-03T10:00:00Z", "Science project score 83."});
    memory.store({"school-02", "maya", "2026-03-12T10:00:00Z", "Science project score corrected to 91."});
    memory.store({"school-03", "sam", "2026-03-13T10:00:00Z", "Science project score 72."});
    const auto start = std::chrono::steady_clock::now();
    auto matches = memory.search({"science project score", "maya", 2, ""});
    const auto elapsed = std::chrono::duration<double, std::milli>(
        std::chrono::steady_clock::now() - start).count();
    std::cout << dmb::offline_report({{"school-correction-sources", {"school-01", "school-02"},
        std::move(matches), elapsed}}) << '\n';
    return 0;
}
