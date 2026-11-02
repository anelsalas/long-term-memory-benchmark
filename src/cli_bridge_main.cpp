// ltm-cli-bridge --settings FILE [--port N] [--failures DIR]
// Serves loopback chat completions through the owner's signed-in Claude Code or
// Codex CLI, for benchmark runs with ltm-compare's local execution mode.
#include "long_term_memory_benchmark/cli_bridge_host.hpp"
#include <atomic>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <sstream>
#include <string>

namespace {
std::string read_file(const std::filesystem::path& path) {
    std::ifstream in(path, std::ios::binary);
    if (!in) throw std::runtime_error("Cannot read " + path.string());
    std::ostringstream text;
    text << in.rdbuf();
    return text.str();
}
}

int main(int argc, char** argv) {
    try {
        std::filesystem::path settings_path;
        unsigned short port = 8773;
        std::filesystem::path failures;
        for (int i = 1; i + 1 < argc; i += 2) {
            const std::string flag = argv[i];
            if (flag == "--settings") settings_path = argv[i + 1];
            else if (flag == "--port") port = static_cast<unsigned short>(std::stoul(argv[i + 1]));
            else if (flag == "--failures") failures = argv[i + 1];
            else throw std::invalid_argument("Unknown option " + flag);
        }
        if (settings_path.empty() || argc % 2 == 0) throw std::invalid_argument("Usage: ltm-cli-bridge --settings FILE [--port N] [--failures DIR]");
        const auto settings = ltm::cli_bridge::parse_settings(read_file(settings_path));
        // The CLIs run in an empty folder so there is nothing near them to read.
        std::filesystem::create_directories(settings.working_directory);
        if (!std::filesystem::is_empty(settings.working_directory))
            throw std::invalid_argument("working_directory must be empty");
        // Codex reads its system prompt from a file; write it so both CLIs get the same text.
        std::ofstream(settings.codex_instructions_file, std::ios::binary | std::ios::trunc) << settings.system_prompt;
        if (read_file(settings.codex_instructions_file) != settings.system_prompt)
            throw std::runtime_error("Cannot write codex_instructions_file");

        ltm::cli_bridge::WindowsCommandRunner runner;
        ltm::cli_bridge::Bridge bridge(settings, runner);
        // Each failed run's output goes to its own file, for diagnosis.
        std::atomic<unsigned> failure_count{0};
        if (!failures.empty()) {
            std::filesystem::create_directories(failures);
            bridge.on_failure([&](const ltm::cli_bridge::Profile& profile, const std::string& code,
                                  const ltm::cli_bridge::RunResult& run) {
                const auto n = ++failure_count;
                std::ofstream(failures / ("failure-" + std::to_string(n) + ".txt"), std::ios::binary)
                    << "profile: " << profile.id << "\ncode: " << code << "\nexit_code: " << run.exit_code
                    << "\ntimed_out: " << run.timed_out << "\n--- output ---\n" << run.output;
                std::cerr << "failure " << n << ' ' << profile.id << ' ' << code << std::endl;
            });
        }
        ltm::cli_bridge::Server server(bridge, port);
        std::cout << "ltm-cli-bridge listening on http://127.0.0.1:" << server.port() << "/v1/chat/completions"
                  << std::endl;
        server.run();
        return 0;
    } catch (const std::exception& e) {
        std::cerr << "ltm-cli-bridge: " << e.what() << '\n';
        return 2;
    }
}
