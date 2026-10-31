#pragma once
#include "long_term_memory_benchmark/dashboard_settings.hpp"
#include <filesystem>
#include <memory>
#include <optional>
#include <string>
#include <utility>
#include <vector>

namespace ltm::dashboard {
// Port: one launched benchmark process.
class Process {
  public:
    virtual ~Process() = default;
    virtual std::optional<int> exit_code() = 0;
    virtual void stop() = 0;
};
// Port: starts the benchmark command with its output captured to a file.
class Launcher {
  public:
    virtual ~Launcher() = default;
    virtual std::unique_ptr<Process> start(const std::vector<std::string>& command,
        const std::filesystem::path& output) = 0;
};

enum class RunStatus { running, completed, failed, stopped, interrupted };
struct RunInfo {
    std::string id;
    RunStatus status = RunStatus::running;
    std::optional<int> exit_code;
};

// Starts one benchmark run at a time; each run lives in runs_dir/<id>/.
class RunControl {
  public:
    RunControl(Launcher& launcher, std::filesystem::path runs_dir, std::vector<std::string> command);
    std::string start(const Settings& settings);
    void stop(const std::string& id);
    std::vector<RunInfo> runs();
  private:
    Launcher& launcher_;
    std::filesystem::path runs_dir_;
    std::vector<std::string> command_;
    struct Run {
        std::string id;
        std::unique_ptr<Process> process;  // null for runs found on disk
        bool stop_requested = false;
        std::optional<RunInfo> recorded;   // final status once written to disk
    };
    RunInfo observe(Run& run);
    std::vector<Run> runs_;
    std::uint64_t next_number_ = 1;
};
}
