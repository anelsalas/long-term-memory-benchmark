#include "long_term_memory_benchmark/dashboard_windows_launcher.hpp"
#include <chrono>
#include <filesystem>
#include <fstream>
#include <functional>
#include <iostream>
#include <optional>
#include <sstream>
#include <stdexcept>
#include <string>
#include <thread>
#include <utility>
#include <vector>

namespace {
using namespace ltm::dashboard;

void require(bool condition, const std::string& message) {
    if (!condition) throw std::runtime_error(message);
}
void quoting() {
    require(windows_command_line({"a.exe", "--offline-demo"}) == L"a.exe --offline-demo", "Plain arguments stay bare");
    require(windows_command_line({"C:\\Program Files\\b.exe", "two words"}) ==
        L"\"C:\\Program Files\\b.exe\" \"two words\"", "Spaces are quoted");
    require(windows_command_line({"a.exe", ""}) == L"a.exe \"\"", "Empty argument is kept");
    require(windows_command_line({"a.exe", "say \"hi\""}) == L"a.exe \"say \\\"hi\\\"\"", "Quotes are escaped");
    require(windows_command_line({"a.exe", "dir\\ x\\"}) == L"a.exe \"dir\\ x\\\\\"", "Trailing backslashes double before closing quote");
    require(windows_command_line({"a.exe", "a\\\\\"b"}) == L"a.exe \"a\\\\\\\\\\\"b\"", "Backslashes before a quote double");
    require(windows_command_line({"a.exe", "\xC3\xA9t\xC3\xA9"}) == L"a.exe \x00E9t\x00E9", "UTF-8 becomes UTF-16");
}
struct TempDir {
    std::filesystem::path path = std::filesystem::temp_directory_path() / ("ltm-launch-" + std::to_string(
        std::chrono::steady_clock::now().time_since_epoch().count()));
    TempDir() { std::filesystem::create_directories(path); }
    ~TempDir() { std::error_code ignored; std::filesystem::remove_all(path, ignored); }
};
std::optional<int> wait_exit(Process& process, std::chrono::milliseconds limit) {
    const auto deadline = std::chrono::steady_clock::now() + limit;
    while (std::chrono::steady_clock::now() < deadline) {
        if (const auto exit = process.exit_code()) return exit;
        std::this_thread::sleep_for(std::chrono::milliseconds(20));
    }
    return std::nullopt;
}
std::string read_file(const std::filesystem::path& file) {
    std::ifstream in(file, std::ios::binary); std::ostringstream text; text << in.rdbuf(); return text.str();
}
void captures_output_and_exit() {
    const TempDir dir; WindowsLauncher launcher;
    const auto output = dir.path / "output.txt";
    auto process = launcher.start({LTM_DASHBOARD_CHILD, "print", "hello dashboard", "exit", "7"}, output);
    require(process != nullptr, "Process starts");
    require(wait_exit(*process, std::chrono::seconds(10)) == 7, "Exit code passes through");
    require(read_file(output).find("hello dashboard") != std::string::npos, "Output is captured to the file");
}
void wait_for_text(const std::filesystem::path& file, const std::string& text) {
    const auto deadline = std::chrono::steady_clock::now() + std::chrono::seconds(10);
    while (read_file(file).find(text) == std::string::npos) {
        require(std::chrono::steady_clock::now() < deadline, "Child printed " + text);
        std::this_thread::sleep_for(std::chrono::milliseconds(20));
    }
}
void stop_ends_process() {
    const TempDir dir; WindowsLauncher launcher;
    const auto output = dir.path / "output.txt";
    auto process = launcher.start({LTM_DASHBOARD_CHILD, "print", "started", "sleep", "30000"}, output);
    wait_for_text(output, "started");
    require(!process->exit_code(), "Child is still running before stop");
    process->stop();
    const auto exit = wait_exit(*process, std::chrono::seconds(5));
    require(exit && *exit != 0, "Stop ends the child promptly with a nonzero code");
}
void dropping_process_kills_child() {
    const TempDir dir; WindowsLauncher launcher;
    const auto output = dir.path / "output.txt";
    auto process = launcher.start({LTM_DASHBOARD_CHILD, "print", "started", "sleep", "500", "print", "late"}, output);
    wait_for_text(output, "started");
    process.reset();
    std::this_thread::sleep_for(std::chrono::milliseconds(1500));
    require(read_file(output).find("late") == std::string::npos, "Child dies with its process object");
}
void runs_offline_demo() {
    const TempDir dir; WindowsLauncher launcher;
    RunControl control(launcher, dir.path, {LTM_BENCHMARK_EXE, "--offline-demo"});
    Settings s;
    s.endpoint_url = "http://localhost:8080/v1";
    s.reader = {"local", "reader", "1"}; s.judge = {"local", "judge", "1"};
    s.max_requests = 1; s.max_tokens = 1; s.max_cost_nanos = 0; s.seed = 1;
    const auto id = control.start(s);
    const auto deadline = std::chrono::steady_clock::now() + std::chrono::seconds(30);
    while (control.runs()[0].status == RunStatus::running) {
        require(std::chrono::steady_clock::now() < deadline, "Offline demo finishes");
        std::this_thread::sleep_for(std::chrono::milliseconds(50));
    }
    require(control.runs()[0].status == RunStatus::completed, "Offline demo completes");
    require(read_file(dir.path / id / "output.txt").find("offline_component") != std::string::npos,
        "Benchmark report is captured in the run folder");
}
}

int main(int argc, char** argv) {
    const std::vector<std::pair<std::string, std::function<void()>>> cases = {
        {"quoting", quoting}, {"captures_output_and_exit", captures_output_and_exit},
        {"stop_ends_process", stop_ends_process}, {"dropping_process_kills_child", dropping_process_kills_child},
        {"runs_offline_demo", runs_offline_demo}};
    if (argc != 2) return 2;
    for (const auto& item : cases) {
        if (item.first == argv[1]) {
            try { item.second(); std::cout << "PASS " << item.first << '\n'; return 0; }
            catch (const std::exception& e) { std::cerr << "FAIL " << item.first << ": " << e.what() << '\n'; return 1; }
        }
    }
    return 2;
}
