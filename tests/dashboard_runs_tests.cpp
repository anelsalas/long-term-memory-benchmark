#include "long_term_memory_benchmark/dashboard_runs.hpp"
#include <chrono>
#include <filesystem>
#include <functional>
#include <iostream>
#include <stdexcept>
#include <string>
#include <utility>
#include <vector>

namespace {
using namespace ltm::dashboard;

void require(bool condition, const std::string& message) {
    if (!condition) throw std::runtime_error(message);
}
struct TempDir {
    std::filesystem::path path = std::filesystem::temp_directory_path() / ("ltm-runs-" + std::to_string(
        std::chrono::steady_clock::now().time_since_epoch().count()));
    TempDir() { std::filesystem::create_directories(path); }
    ~TempDir() { std::error_code ignored; std::filesystem::remove_all(path, ignored); }
};
struct FakeProcess : Process {
    std::optional<int> exit;
    bool stopped = false;
    std::optional<int> exit_code() override { return exit; }
    void stop() override { stopped = true; exit = 1; }
};
struct FakeLauncher : Launcher {
    std::vector<std::string> command;
    std::filesystem::path output;
    std::vector<FakeProcess*> started;
    std::unique_ptr<Process> start(const std::vector<std::string>& c, const std::filesystem::path& o) override {
        command = c; output = o;
        auto process = std::make_unique<FakeProcess>();
        started.push_back(process.get());
        return process;
    }
};
Settings settings() {
    Settings s;
    s.endpoint_url = "http://localhost:8080/v1";
    s.reader = {"local", "reader", "1"}; s.judge = {"local", "judge", "1"};
    s.max_requests = 4; s.max_tokens = 1000; s.max_cost_nanos = 0; s.seed = 7;
    return s;
}

void start_launches_command() {
    const TempDir dir; FakeLauncher launcher;
    RunControl control(launcher, dir.path, {"benchmark.exe", "--offline-demo"});
    const auto id = control.start(settings());
    require(id == "run-1", "First run is run-1");
    require(launcher.command == std::vector<std::string>{"benchmark.exe", "--offline-demo"}, "Configured command is launched");
    require(launcher.output == dir.path / "run-1" / "output.txt", "Output is captured in the run folder");
    const auto saved = load_settings(dir.path / "run-1" / "settings.json");
    require(saved && saved->seed == 7, "Settings used by the run are recorded with it");
    const auto runs = control.runs();
    require(runs.size() == 1 && runs[0].id == "run-1" && runs[0].status == RunStatus::running, "New run is running");
}
template<class F> bool refused(F action) {
    try { action(); } catch (const std::logic_error&) { return true; }
    return false;
}
void one_run_at_a_time() {
    const TempDir dir; FakeLauncher launcher;
    RunControl control(launcher, dir.path, {"benchmark.exe"});
    control.start(settings());
    require(refused([&] { control.start(settings()); }), "Second start is refused while a run is active");
    require(launcher.started.size() == 1 && control.runs().size() == 1, "Refused start launches nothing");
    launcher.started[0]->exit = 0;
    require(control.start(settings()) == "run-2", "A new run may start once the previous one ends");
}
void status_follows_exit() {
    const TempDir dir; FakeLauncher launcher;
    RunControl control(launcher, dir.path, {"benchmark.exe"});
    control.start(settings());
    launcher.started[0]->exit = 0;
    control.start(settings());
    launcher.started[1]->exit = 3;
    const auto runs = control.runs();
    require(runs[0].status == RunStatus::completed && runs[0].exit_code == 0, "Exit 0 means completed");
    require(runs[1].status == RunStatus::failed && runs[1].exit_code == 3, "Nonzero exit means failed");
}
void stop_run() {
    const TempDir dir; FakeLauncher launcher;
    RunControl control(launcher, dir.path, {"benchmark.exe"});
    const auto first = control.start(settings());
    launcher.started[0]->exit = 0;
    control.stop(first);
    require(!launcher.started[0]->stopped && control.runs()[0].status == RunStatus::completed,
        "Stopping a finished run changes nothing");
    const auto second = control.start(settings());
    control.stop(second);
    require(launcher.started[1]->stopped, "Stop ends the process");
    require(control.runs()[1].status == RunStatus::stopped, "A stopped run reports stopped, not failed");
    bool unknown = false;
    try { control.stop("run-9"); } catch (const std::invalid_argument&) { unknown = true; }
    require(unknown, "Unknown run IDs are rejected");
}
void survives_restart() {
    const TempDir dir; FakeLauncher launcher;
    {
        RunControl control(launcher, dir.path, {"benchmark.exe"});
        control.start(settings()); launcher.started[0]->exit = 0;
        control.start(settings()); launcher.started[1]->exit = 2;
        control.start(settings()); control.stop("run-3");
        control.runs();
        control.start(settings());
    }
    RunControl restarted(launcher, dir.path, {"benchmark.exe"});
    const auto runs = restarted.runs();
    require(runs.size() == 4, "Earlier runs are listed after restart");
    require(runs[0].status == RunStatus::completed && runs[0].exit_code == 0, "Completed run keeps its status");
    require(runs[1].status == RunStatus::failed && runs[1].exit_code == 2, "Failed run keeps its exit code");
    require(runs[2].status == RunStatus::stopped, "Stopped run keeps its status");
    require(runs[3].status == RunStatus::interrupted && !runs[3].exit_code, "Unfinished run is interrupted");
    require(restarted.start(settings()) == "run-5", "IDs continue after the highest existing run");
    std::filesystem::create_directories(dir.path / "run-12");
    std::filesystem::create_directories(dir.path / "notes");
    RunControl again(launcher, dir.path, {"benchmark.exe"});
    require(again.runs().size() == 6, "Only run-N folders are runs");
    launcher.started.back()->exit = 0;
    require(again.start(settings()) == "run-13", "Gaps never cause an ID to be reused");
}
}

int main(int argc, char** argv) {
    const std::vector<std::pair<std::string, std::function<void()>>> cases = {
        {"start_launches_command", start_launches_command},
        {"one_run_at_a_time", one_run_at_a_time},
        {"status_follows_exit", status_follows_exit}, {"stop_run", stop_run},
        {"survives_restart", survives_restart}};
    if (argc != 2) return 2;
    for (const auto& item : cases) {
        if (item.first == argv[1]) {
            try { item.second(); std::cout << "PASS " << item.first << '\n'; return 0; }
            catch (const std::exception& e) { std::cerr << "FAIL " << item.first << ": " << e.what() << '\n'; return 1; }
        }
    }
    return 2;
}
