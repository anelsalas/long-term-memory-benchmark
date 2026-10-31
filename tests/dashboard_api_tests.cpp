#include "long_term_memory_benchmark/dashboard_api.hpp"
#include <nlohmann/json.hpp>
#include <chrono>
#include <filesystem>
#include <fstream>
#include <functional>
#include <iostream>
#include <stdexcept>
#include <string>
#include <utility>
#include <vector>

namespace {
using namespace ltm::dashboard;
using Json = nlohmann::json;

void require(bool condition, const std::string& message) {
    if (!condition) throw std::runtime_error(message);
}
struct TempDir {
    std::filesystem::path path = std::filesystem::temp_directory_path() / ("ltm-api-" + std::to_string(
        std::chrono::steady_clock::now().time_since_epoch().count()));
    TempDir() { std::filesystem::create_directories(path); }
    ~TempDir() { std::error_code ignored; std::filesystem::remove_all(path, ignored); }
};
struct FakeProcess : Process {
    std::optional<int> exit;
    std::optional<int> exit_code() override { return exit; }
    void stop() override { exit = 1; }
};
struct FakeLauncher : Launcher {
    std::vector<FakeProcess*> started;
    std::unique_ptr<Process> start(const std::vector<std::string>&, const std::filesystem::path& output) override {
        std::ofstream(output) << "benchmark output line\n";
        auto process = std::make_unique<FakeProcess>();
        started.push_back(process.get());
        return process;
    }
};
// One dashboard wired to fakes, with a web folder holding index.html.
struct Fixture {
    TempDir dir;
    FakeLauncher launcher;
    RunControl runs{launcher, dir.path / "runs", {"benchmark.exe", "--offline-demo"}};
    Api api{dir.path, runs, dir.path / "web"};
    Fixture() {
        std::filesystem::create_directories(dir.path / "web");
        std::ofstream(dir.path / "web" / "index.html") << "<!doctype html><title>Dashboard</title>";
    }
    Response get(const std::string& target) { return api.handle({"GET", target, "", ""}); }
    Response send(const std::string& method, const std::string& target, const std::string& body = "{}") {
        return api.handle({method, target, "application/json", body});
    }
};
const std::string valid_settings = R"({"schema_version":1,"endpoint_url":"http://localhost:8080/v1",
    "reader":{"provider":"local","id":"reader","version":"1"},"judge":{"provider":"local","id":"judge","version":"1"},
    "max_requests":"4","max_tokens":"1000","max_cost_nanos":"0","seed":"7"})";

void settings_routes() {
    Fixture f;
    auto r = f.get("/api/settings");
    require(r.status == 200 && Json::parse(r.body).at("settings").is_null(), "No settings saved yet");
    r = f.send("PUT", "/api/settings", valid_settings);
    require(r.status == 200 && Json::parse(r.body).at("settings").at("seed") == "7", "Valid settings are saved");
    r = f.get("/api/settings");
    require(Json::parse(r.body).at("settings").at("reader").at("id") == "reader", "Saved settings are returned");
    r = f.send("PUT", "/api/settings", R"({"schema_version":1})");
    require(r.status == 400 && !Json::parse(r.body).at("error").get<std::string>().empty(), "Invalid settings explain why");
    require(Json::parse(f.get("/api/settings").body).at("settings").at("seed") == "7", "Rejected settings change nothing");
    r = f.api.handle({"PUT", "/api/settings", "text/plain", valid_settings});
    require(r.status == 415, "Changes must be sent as JSON");
}
void run_routes() {
    Fixture f;
    auto r = f.send("POST", "/api/runs");
    require(r.status == 409 && f.launcher.started.empty(), "A run needs saved settings first");
    f.send("PUT", "/api/settings", valid_settings);
    r = f.send("POST", "/api/runs");
    require(r.status == 201 && Json::parse(r.body).at("run").at("id") == "run-1", "Start returns the new run");
    require(f.send("POST", "/api/runs").status == 409, "Only one run at a time");
    r = f.get("/api/runs");
    const auto listed = Json::parse(r.body).at("runs");
    require(listed.size() == 1 && listed[0].at("status") == "running" && listed[0].at("exit_code").is_null(),
        "Runs list shows status");
    require(f.send("POST", "/api/runs/run-1/stop").status == 200, "Stop is accepted");
    require(Json::parse(f.get("/api/runs").body).at("runs")[0].at("status") == "stopped", "Stopped run is listed as stopped");
    require(f.send("POST", "/api/runs/run-9/stop").status == 404, "Unknown run is not found");
    require(f.api.handle({"POST", "/api/runs", "text/plain", "{}"}).status == 415, "Start must be sent as JSON");
}
void output_route() {
    Fixture f;
    f.send("PUT", "/api/settings", valid_settings);
    f.send("POST", "/api/runs");
    auto r = f.get("/api/runs/run-1/output");
    require(r.status == 200 && r.content_type.rfind("text/plain", 0) == 0, "Output is plain text");
    require(r.body.find("benchmark output line") != std::string::npos, "Output shows what the benchmark printed");
    require(f.get("/api/runs/run-9/output").status == 404, "Unknown run has no output");
    require(f.get("/api/runs/../settings.json/output").status == 404, "Run IDs cannot reach other files");
}
void web_files() {
    Fixture f;
    std::ofstream(f.dir.path / "web" / "app.js") << "export {};";
    std::ofstream(f.dir.path / "web" / "notes.txt") << "private";
    auto r = f.get("/");
    require(r.status == 200 && r.content_type.rfind("text/html", 0) == 0 && r.body.find("Dashboard") != std::string::npos,
        "Root serves index.html");
    r = f.get("/app.js");
    require(r.status == 200 && r.content_type.rfind("text/javascript", 0) == 0, "Scripts are served as JavaScript");
    require(f.get("/missing.js").status == 404, "Missing file is not found");
    require(f.get("/notes.txt").status == 404, "Only page file types are served");
    require(f.get("/../settings.json").status == 404 && f.get("/runs/run-1/output.txt").status == 404,
        "Nothing outside the web folder is served");
    require(f.get("/%2e%2e/settings.json").status == 404 && f.get("/app.js?v=1").status == 200,
        "Encoded paths are refused; query strings are ignored");
    require(f.send("POST", "/").status == 405, "Page files are read-only");
    require(f.get("/api/unknown").status == 404, "Unknown API route is not found");
}
}

int main(int argc, char** argv) {
    const std::vector<std::pair<std::string, std::function<void()>>> cases = {
        {"settings_routes", settings_routes}, {"run_routes", run_routes},
        {"output_route", output_route}, {"web_files", web_files}};
    if (argc != 2) return 2;
    for (const auto& item : cases) {
        if (item.first == argv[1]) {
            try { item.second(); std::cout << "PASS " << item.first << '\n'; return 0; }
            catch (const std::exception& e) { std::cerr << "FAIL " << item.first << ": " << e.what() << '\n'; return 1; }
        }
    }
    return 2;
}
