#include "long_term_memory_benchmark/cli_bridge_host.hpp"
#include <boost/asio.hpp>
#include <boost/beast.hpp>
#include <nlohmann/json.hpp>
#include <chrono>
#include <cstdlib>
#include <filesystem>
#include <fstream>
#include <functional>
#include <iostream>
#include <stdexcept>
#include <string>
#include <thread>
#include <utility>
#include <vector>

namespace {
namespace asio = boost::asio;
namespace http = boost::beast::http;
using namespace ltm::cli_bridge;
using Json = nlohmann::json;
std::string executable;

void require(bool condition, const std::string& message) {
    if (!condition) throw std::runtime_error(message);
}
std::string system_tool(const char* name) {
    const char* root = std::getenv("SystemRoot");
    return (std::filesystem::path(root ? root : "C:\\Windows") / "System32" / name).string();
}

void runner_passes_input_and_folder() {
    WindowsCommandRunner runner;
    const auto sorted = runner.run({{system_tool("sort.exe")}, "pear\r\napple\r\n", "", 10000});
    require(!sorted.timed_out && sorted.exit_code == 0 && sorted.output == "apple\r\npear\r\n", "Input reaches the child");
    const auto folder = std::filesystem::temp_directory_path();
    const auto cwd = runner.run({{system_tool("cmd.exe"), "/d", "/c", "cd"}, "", folder.string(), 10000});
    require(std::filesystem::equivalent(cwd.output.substr(0, cwd.output.find('\r')), folder), "Runs in the given folder");
}
void runner_reports_exit_codes() {
    WindowsCommandRunner runner;
    const auto result = runner.run({{system_tool("cmd.exe"), "/d", "/c", "exit 3"}, "", "", 10000});
    require(!result.timed_out && result.exit_code == 3, "The exit code is reported");
}
void runner_stops_at_timeout() {
    WindowsCommandRunner runner;
    const auto start = std::chrono::steady_clock::now();
    const auto result = runner.run({{system_tool("PING.EXE"), "-n", "30", "127.0.0.1"}, "", "", 300});
    require(result.timed_out, "A slow child times out");
    require(std::chrono::steady_clock::now() - start < std::chrono::seconds(10), "...and is stopped promptly");
}
void runner_streams_large_input() {
    WindowsCommandRunner runner;
    std::string input;
    for (int i = 0; i < 200000; ++i) input += "same line\r\n";
    const auto result = runner.run({{system_tool("sort.exe")}, input, "", 60000});
    require(!result.timed_out && result.output == input, "Megabytes flow both ways without blocking");
}
void runner_uses_isolated_instructions() {
    WindowsCommandRunner runner;
    Invocation invocation{{executable, "instructions_child", "model_instructions_file=placeholder"}, "", "", 10000};
    invocation.system_prompt = "extract facts as JSON";
    const auto result = runner.run(invocation);
    require(!result.timed_out && result.exit_code == 0, "Instruction reader runs");
    const auto newline = result.output.find('\n');
    require(newline != std::string::npos && result.output.substr(newline + 1) == *invocation.system_prompt,
        "Child receives the system instruction in its own file");
    require(!std::filesystem::exists(result.output.substr(0, newline)), "Temporary instructions are removed");
}

class FakeRunner : public CommandRunner {
  public:
    RunResult run(const Invocation& invocation) override {
        ++calls;
        return {0, R"({"type":"result","subtype":"success","is_error":false,"result":"echo:)" + invocation.input +
            R"(","usage":{"input_tokens":5,"output_tokens":2},"modelUsage":{"claude-haiku-5-5":{}}})", false};
    }
    int calls = 0;
};
Settings settings() {
    return parse_settings(R"({"claude_executable":"claude.exe","codex_executable":"codex.exe",
      "system_prompt":"s","codex_instructions_file":"i.txt","working_directory":".","timeout_ms":1000,
      "profiles":[{"id":"haiku","cli":"claude","model":"claude-haiku-5-5","effort":"low"}]})");
}
struct Running {
    explicit Running(Bridge& bridge) : server(bridge, 0), thread([this] { server.run(); }) {}
    ~Running() { server.stop(); thread.join(); }
    Server server;
    std::thread thread;
};
http::response<http::string_body> send(unsigned short port, http::verb verb, const std::string& target,
                                       const std::string& body, const std::string& host, const std::string& origin = "") {
    asio::io_context io;
    asio::ip::tcp::socket socket(io);
    socket.connect({asio::ip::make_address("127.0.0.1"), port});
    http::request<http::string_body> request{verb, target, 11};
    request.set(http::field::host, host);
    if (!origin.empty()) request.set(http::field::origin, origin);
    request.set(http::field::content_type, "application/json");
    request.body() = body;
    request.prepare_payload();
    http::write(socket, request);
    boost::beast::flat_buffer buffer;
    http::response<http::string_body> response;
    http::read(socket, buffer, response);
    return response;
}
std::string chat(const std::string& prompt) {
    return Json{{"model", "haiku"}, {"messages", {{{"role", "user"}, {"content", prompt}}}}, {"stream", false}}.dump();
}

void server_serves_chat_completions() {
    FakeRunner runner;
    Bridge bridge(settings(), runner);
    Running running(bridge);
    const auto port = running.server.port();
    const auto host = "127.0.0.1:" + std::to_string(port);
    const auto health = send(port, http::verb::get, "/health", "", host);
    require(health.result() == http::status::ok, "Health check");
    const auto reply = send(port, http::verb::post, "/v1/chat/completions", chat("hi"), host);
    require(reply.result() == http::status::ok, "Chat completion");
    require(Json::parse(reply.body())["choices"][0]["message"]["content"] == "echo:hi", "The bridge answers");
    const auto metrics = send(port, http::verb::get, "/metrics", "", host);
    require(metrics.result() == http::status::ok && Json::parse(metrics.body())["attempts"] == 1 &&
        Json::parse(metrics.body())["input_tokens"] == 5, "Usage metrics are available to local clients");
    std::vector<std::thread> clients;
    std::vector<unsigned> statuses(4);
    for (std::size_t i = 0; i < statuses.size(); ++i)
        clients.emplace_back([&, i] { statuses[i] = send(port, http::verb::post, "/v1/chat/completions", chat("x"), host).result_int(); });
    for (auto& client : clients) client.join();
    for (const auto status : statuses) require(status == 200, "Concurrent requests are all answered");
    require(send(port, http::verb::post, "/other", chat("hi"), host).result() == http::status::not_found, "Other paths");
}
void server_refuses_browsers_and_foreign_hosts() {
    FakeRunner runner;
    Bridge bridge(settings(), runner);
    Running running(bridge);
    const auto port = running.server.port();
    const auto host = "127.0.0.1:" + std::to_string(port);
    require(send(port, http::verb::post, "/v1/chat/completions", chat("hi"), host, "http://example.com").result() ==
        http::status::forbidden, "Browser requests are refused");
    require(send(port, http::verb::post, "/v1/chat/completions", chat("hi"), "example.com").result() ==
        http::status::forbidden, "Foreign Host headers are refused");
    require(runner.calls == 0, "Refused requests start nothing");
}
}

int main(int argc, char** argv) {
    if (argc == 3 && std::string(argv[1]) == "instructions_child") {
        const std::string arg = argv[2];
        const auto first = arg.find('"');
        const auto last = arg.rfind('"');
        if (first == std::string::npos || last <= first) return 3;
        const auto path = arg.substr(first + 1, last - first - 1);
        std::ifstream in(path, std::ios::binary);
        if (!in) return 4;
        std::cout << path << '\n' << in.rdbuf();
        return 0;
    }
    executable = std::filesystem::absolute(argv[0]).string();
    const std::vector<std::pair<std::string, std::function<void()>>> cases = {
        {"runner_passes_input_and_folder", runner_passes_input_and_folder},
        {"runner_reports_exit_codes", runner_reports_exit_codes}, {"runner_stops_at_timeout", runner_stops_at_timeout},
        {"runner_streams_large_input", runner_streams_large_input},
        {"runner_uses_isolated_instructions", runner_uses_isolated_instructions},
        {"server_serves_chat_completions", server_serves_chat_completions},
        {"server_refuses_browsers_and_foreign_hosts", server_refuses_browsers_and_foreign_hosts}};
    if (argc != 2) return 2;
    for (const auto& item : cases) {
        if (item.first == argv[1]) {
            try { item.second(); std::cout << "PASS " << item.first << '\n'; return 0; }
            catch (const std::exception& e) { std::cerr << "FAIL " << item.first << ": " << e.what() << '\n'; return 1; }
        }
    }
    return 2;
}
