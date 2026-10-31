#include "long_term_memory_benchmark/dashboard_runs.hpp"
#include <algorithm>
#include <fstream>
#include <map>
#include <stdexcept>

namespace ltm::dashboard {
namespace {
const std::map<RunStatus, std::string> status_names = {{RunStatus::completed, "completed"},
    {RunStatus::failed, "failed"}, {RunStatus::stopped, "stopped"}};

std::optional<std::uint64_t> run_number(const std::string& name) {
    if (name.size() < 5 || name.size() > 22 || name.rfind("run-", 0) != 0 || name[4] == '0') return std::nullopt;
    std::uint64_t n = 0;
    for (auto c = name.begin() + 4; c != name.end(); ++c) {
        if (*c < '0' || *c > '9') return std::nullopt;
        n = n * 10 + static_cast<std::uint64_t>(*c - '0');
    }
    return n;
}
// A finished run's folder holds status.txt: "<status> <exit code>". Anything else was interrupted.
RunInfo read_recorded(const std::filesystem::path& folder, const std::string& id) {
    std::ifstream in(folder / "status.txt");
    std::string name; int exit = 0;
    if (in >> name >> exit)
        for (const auto& [status, text] : status_names)
            if (text == name) return {id, status, exit};
    return {id, RunStatus::interrupted, std::nullopt};
}
void write_recorded(const std::filesystem::path& folder, const RunInfo& info) {
    std::ofstream out(folder / "status.txt", std::ios::trunc);
    out << status_names.at(info.status) << ' ' << *info.exit_code << '\n';
}
}

RunControl::RunControl(Launcher& launcher, std::filesystem::path runs_dir, std::vector<std::string> command)
    : launcher_(launcher), runs_dir_(std::move(runs_dir)), command_(std::move(command)) {
    std::vector<std::pair<std::uint64_t, std::string>> found;
    if (std::filesystem::is_directory(runs_dir_))
        for (const auto& entry : std::filesystem::directory_iterator(runs_dir_)) {
            const auto name = entry.path().filename().string();
            if (const auto n = run_number(name); n && entry.is_directory()) found.emplace_back(*n, name);
        }
    std::sort(found.begin(), found.end());
    for (const auto& [number, id] : found) {
        runs_.push_back({id, nullptr, false, read_recorded(runs_dir_ / id, id)});
        next_number_ = number + 1;
    }
}

std::string RunControl::start(const Settings& settings) {
    for (auto& run : runs_)
        if (observe(run).status == RunStatus::running) throw std::logic_error("A benchmark run is already active");
    const std::string id = "run-" + std::to_string(next_number_);
    const auto folder = runs_dir_ / id;
    std::filesystem::create_directories(folder);
    save_settings(folder / "settings.json", settings);
    runs_.push_back({id, launcher_.start(command_, folder / "output.txt")});
    ++next_number_;
    return id;
}
void RunControl::stop(const std::string& id) {
    const auto run = std::find_if(runs_.begin(), runs_.end(), [&](const Run& r) { return r.id == id; });
    if (run == runs_.end()) throw std::invalid_argument("Unknown run: " + id);
    if (observe(*run).status != RunStatus::running) return;
    run->stop_requested = true;
    run->process->stop();
}
std::filesystem::path RunControl::output_file(const std::string& id) const {
    const auto known = std::any_of(runs_.begin(), runs_.end(), [&](const Run& r) { return r.id == id; });
    if (!known) throw std::invalid_argument("Unknown run: " + id);
    return runs_dir_ / id / "output.txt";
}
std::vector<RunInfo> RunControl::runs() {
    std::vector<RunInfo> result;
    for (auto& run : runs_) result.push_back(observe(run));
    return result;
}
RunInfo RunControl::observe(Run& run) {
    if (run.recorded) return *run.recorded;
    const auto exit = run.process->exit_code();
    if (!exit) return {run.id, RunStatus::running, std::nullopt};
    const auto status = run.stop_requested ? RunStatus::stopped : *exit == 0 ? RunStatus::completed : RunStatus::failed;
    run.recorded = RunInfo{run.id, status, exit};
    write_recorded(runs_dir_ / run.id, *run.recorded);
    return *run.recorded;
}
}
