#pragma once
#include "long_term_memory_benchmark/dashboard_runs.hpp"
#include <string>
#include <vector>

namespace ltm::dashboard {
// Quotes UTF-8 arguments into one Windows command line (CommandLineToArgvW rules).
std::wstring windows_command_line(const std::vector<std::string>& command);

// Runs each benchmark in a kill-on-close Job Object; only the output file is inherited.
class WindowsLauncher : public Launcher {
  public:
    std::unique_ptr<Process> start(const std::vector<std::string>& command,
        const std::filesystem::path& output) override;
};
}
