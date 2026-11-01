// long-term-memory-dashboard: thin local control panel for benchmark runs.
#include "long_term_memory_benchmark/dashboard_plan.hpp"
#include "long_term_memory_benchmark/dashboard_server.hpp"
#include "long_term_memory_benchmark/dashboard_windows_launcher.hpp"
#include <windows.h>
#include <cstdlib>
#include <exception>
#include <fstream>
#include <iostream>
#include <iterator>
#include <string>
#include <vector>

namespace {
const char* usage =
    "Usage: long-term-memory-dashboard [--bind ADDRESS] [--port PORT] [--data DIR] [--web DIR] [--plan FILE]\n"
    "                                  [--check-plan] [-- COMMAND...]\n"
    "  --bind  listen address (default 127.0.0.1; use a LAN address to allow other devices)\n"
    "  --port  listen port (default 18080)\n"
    "  --data  folder for settings.json and runs/ (default runtime/dashboard)\n"
    "  --web   folder with index.html, app.js and styles.css\n"
    "  --plan  reviewed ltm-compare plan; each run fills in the saved setup (default:\n"
    "          configs/pilot-fresh-plan.example.json)\n"
    "  COMMAND comparison program (default: ltm-compare next to this program). Each run adds\n"
    "          --config <run folder>/config.json --dry-run by default; saved local mode uses --execute.\n"
    "  --check-plan validate the reviewed plan and exit before starting the server.\n";

ltm::dashboard::Server* active_server = nullptr;
BOOL WINAPI on_console_close(DWORD) {
    if (active_server) active_server->stop();
    return TRUE;
}
std::filesystem::path program_folder() {
    std::wstring path(MAX_PATH, L'\0');
    const auto size = GetModuleFileNameW(nullptr, path.data(), static_cast<DWORD>(path.size()));
    path.resize(size);
    return std::filesystem::path(path).parent_path();
}
}

int main(int argc, char** argv) {
    std::string bind = "127.0.0.1";
    unsigned long port = 18080;
    std::filesystem::path data = "runtime/dashboard";
    std::filesystem::path web = LTM_DASHBOARD_WEB_DIR;
    std::filesystem::path plan = LTM_DASHBOARD_PLAN;
    std::vector<std::string> command;
    bool check_plan = false;
    for (int i = 1; i < argc; ++i) {
        const std::string arg = argv[i];
        const bool has_value = i + 1 < argc;
        if (arg == "--help") { std::cout << usage; return 0; }
        if (arg == "--check-plan") { check_plan = true; continue; }
        if (arg == "--") { command.assign(argv + i + 1, argv + argc); break; }
        if (arg == "--bind" && has_value) bind = argv[++i];
        else if (arg == "--port" && has_value) port = std::strtoul(argv[++i], nullptr, 10);
        else if (arg == "--data" && has_value) data = argv[++i];
        else if (arg == "--web" && has_value) web = argv[++i];
        else if (arg == "--plan" && has_value) plan = argv[++i];
        else { std::cerr << usage; return 2; }
    }
    if (port == 0 || port > 65535) { std::cerr << "Port must be 1-65535\n"; return 2; }
    if (command.empty())
        command = {(program_folder() / "ltm-compare.exe").string()};
    try {
        std::ifstream plan_file(plan, std::ios::binary);
        if (!plan_file) throw std::runtime_error("Cannot read plan " + plan.string());
        const std::string plan_template((std::istreambuf_iterator<char>(plan_file)), {});
        ltm::dashboard::validate_compare_plan_template(plan_template);
        if (check_plan) { std::cout << "Plan validated\n"; return 0; }
        std::filesystem::create_directories(data / "runs");
        ltm::dashboard::WindowsLauncher launcher;
        ltm::dashboard::RunControl runs(launcher, data / "runs", command, ltm::dashboard::compare_dry_run(plan_template));
        ltm::dashboard::Api api(data, runs, web);
        ltm::dashboard::Server server(api, bind, static_cast<unsigned short>(port));
        active_server = &server;
        SetConsoleCtrlHandler(on_console_close, TRUE);
        std::cout << "Dashboard: http://" << (bind == "0.0.0.0" ? "127.0.0.1" : bind) << ':' << server.port() << "/\n"
                  << "Data: " << std::filesystem::absolute(data).string() << "\nPlan: " << plan.string()
                  << "\nPress Ctrl+C to stop.\n" << std::flush;
        server.run();
        active_server = nullptr;
        std::cout << "Dashboard stopped; any active run was ended.\n";
    } catch (const std::exception& e) {
        std::cerr << "Dashboard failed: " << e.what() << '\n';
        return 1;
    }
    return 0;
}
