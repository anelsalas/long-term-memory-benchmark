#include "long_term_memory_benchmark/cli_bridge.hpp"
#include "long_term_memory_benchmark/hash.hpp"
#include <nlohmann/json.hpp>
#include <algorithm>
#include <atomic>
#include <chrono>
#include <thread>
#include <functional>
#include <iostream>
#include <stdexcept>
#include <string>
#include <utility>
#include <vector>

namespace {
using namespace ltm::cli_bridge;
using Json = nlohmann::json;

void require(bool condition, const std::string& message) {
    if (!condition) throw std::runtime_error(message);
}
template<class E, class F> bool throws(F action) {
    try { action(); } catch (const E&) { return true; }
    return false;
}
bool contains(const std::vector<std::string>& args, const std::vector<std::string>& run) {
    return std::search(args.begin(), args.end(), run.begin(), run.end()) != args.end();
}

const char* settings_json = R"({
  "claude_executable": "C:/tools/claude.exe", "codex_executable": "C:/tools/codex.exe",
  "system_prompt": "You are a helpful assistant.", "codex_instructions_file": "C:/bridge/instructions.txt",
  "working_directory": "C:/bridge/empty", "timeout_ms": 1000,
  "profiles": [
    {"id": "claude-opus-5-5/high", "cli": "claude", "model": "claude-opus-5-5", "effort": "high"},
    {"id": "gpt-6.1-sol/low", "cli": "codex", "model": "gpt-6.1-sol", "effort": "low"}]})";

const char* claude_success = R"({"type":"result","subtype":"success","is_error":false,"result":"42",
  "usage":{"input_tokens":2,"cache_creation_input_tokens":727,"cache_read_input_tokens":10,"output_tokens":3},
  "modelUsage":{"claude-opus-5-5":{"inputTokens":2,"outputTokens":3}}})";

const char* codex_success =
    "{\"type\":\"thread.started\",\"thread_id\":\"t\"}\n"
    "{\"type\":\"turn.started\"}\n"
    "{\"type\":\"item.completed\",\"item\":{\"id\":\"item_0\",\"type\":\"reasoning\",\"text\":\"thinking\"}}\n"
    "{\"type\":\"item.completed\",\"item\":{\"id\":\"item_1\",\"type\":\"agent_message\",\"text\":\"42\"}}\n"
    "{\"type\":\"turn.completed\",\"usage\":{\"input_tokens\":6547,\"cached_input_tokens\":100,"
    "\"output_tokens\":5,\"reasoning_output_tokens\":2}}\n";

std::string chat(const std::string& model, const std::string& prompt) {
    return Json{{"model", model}, {"messages", {{{"role", "user"}, {"content", prompt}}}},
        {"stream", false}, {"max_tokens", 64}, {"temperature", 0}}.dump();
}

// Returns the given results in turn, repeating the last one.
class FakeRunner : public CommandRunner {
  public:
    explicit FakeRunner(RunResult result) : results_{std::move(result)} {}
    explicit FakeRunner(std::vector<RunResult> results) : results_(std::move(results)) {}
    RunResult run(const Invocation& invocation) override {
        calls.push_back(invocation);
        return results_[std::min(calls.size(), results_.size()) - 1];
    }
    std::vector<Invocation> calls;
  private:
    std::vector<RunResult> results_;
};

void settings_name_profiles() {
    const auto settings = parse_settings(settings_json);
    require(settings.profiles.size() == 2 && settings.profiles[1].cli == Cli::codex, "Both profiles load");
    require(find_profile(settings, "gpt-6.1-sol/low")->model == "gpt-6.1-sol", "Profiles are found by ID");
    require(find_profile(settings, "gpt-6.1-sol") == nullptr, "Only exact IDs match");
    require(throws<std::invalid_argument>([] { parse_settings(R"({"profiles": []})"); }), "Incomplete settings are refused");
    require(throws<std::invalid_argument>([] {
        auto bad = Json::parse(settings_json);
        bad["profiles"][0]["cli"] = "gemini";
        parse_settings(bad.dump());
    }), "Unknown CLIs are refused");
    require(throws<std::invalid_argument>([] {
        auto bad = Json::parse(settings_json);
        bad["max_concurrent"] = -1;
        parse_settings(bad.dump());
    }), "Negative concurrency is refused");
}
void claude_runs_without_tools_or_settings() {
    const auto settings = parse_settings(settings_json);
    const auto args = command(settings, settings.profiles[0]);
    require(args.front() == "C:/tools/claude.exe" && args[1] == "-p", "Claude Code runs in print mode");
    require(contains(args, {"--model", "claude-opus-5-5"}) && contains(args, {"--effort", "high"}), "Model and effort");
    require(contains(args, {"--tools", ""}) && contains(args, {"--setting-sources", ""}) &&
        contains(args, {"--strict-mcp-config"}) && contains(args, {"--no-session-persistence"}),
        "No tools, settings, MCP servers or saved sessions");
    require(contains(args, {"--system-prompt", "You are a helpful assistant."}), "The neutral system prompt replaces the default");
    require(contains(args, {"--output-format", "json"}), "JSON result with usage");
}
void codex_runs_without_tools_or_user_config() {
    const auto settings = parse_settings(settings_json);
    const auto args = command(settings, settings.profiles[1]);
    require(args.front() == "C:/tools/codex.exe" && args[1] == "exec", "Codex runs non-interactively");
    require(contains(args, {"-m", "gpt-6.1-sol"}) && contains(args, {"-c", "model_reasoning_effort=\"low\""}), "Model and effort");
    require(contains(args, {"--ignore-user-config"}) && contains(args, {"--ephemeral"}) && contains(args, {"-s", "read-only"}),
        "No user config, no saved session, read-only sandbox");
    require(contains(args, {"-c", "model_instructions_file='C:/bridge/instructions.txt'"}), "The same system prompt, from a file");
    require(contains(args, {"--disable", "shell_tool"}) && contains(args, {"--disable", "unified_exec"}) &&
        contains(args, {"--disable", "code_mode_host"}), "Command tools are disabled");
    require(args[args.size() - 2] == "--json" && args.back() == "-", "JSON events; the prompt comes from stdin");
}
void claude_result_is_parsed() {
    const auto reply = parse_claude_result(claude_success);
    require(reply.text == "42" && reply.model == "claude-opus-5-5", "Answer and served model");
    require(reply.input_tokens == 739 && reply.output_tokens == 3, "Input counts cached and uncached tokens");
    require(throws<CliError>([] {
        parse_claude_result(R"({"type":"result","subtype":"error_during_execution","is_error":true,"result":"limit"})");
    }), "An error result is an error");
    require(throws<CliError>([] { parse_claude_result("not json"); }), "Unreadable output is an error");
    require(throws<CliError>([] {
        auto two = Json::parse(claude_success);
        two["modelUsage"]["claude-haiku-5-5"] = Json::object();
        parse_claude_result(two.dump());
    }), "One run must use one model");
}
void codex_events_are_parsed() {
    const auto reply = parse_codex_events(codex_success);
    require(reply.text == "42" && reply.model.empty(), "Last answer; Codex does not report the served model");
    require(reply.input_tokens == 6547 && reply.output_tokens == 5, "Usage from the completed turn");
    const std::string notice = "{\"type\":\"item.completed\",\"item\":{\"id\":\"item_9\",\"type\":\"error\","
        "\"message\":\"Code Mode is unavailable because code-mode host is disabled.\"}}\n" + std::string(codex_success);
    require(parse_codex_events(notice).text == "42", "Notices about disabled features are not tool use");
    const std::string reconnect = "{\"type\":\"error\",\"message\":\"Reconnecting... 3/5 (unexpected status 403)\"}\n" +
        std::string(codex_success);
    require(parse_codex_events(reconnect).text == "42", "A reconnect notice before a completed turn is not a failure");
    const std::string tool = std::string(codex_success) +
        "{\"type\":\"item.completed\",\"item\":{\"id\":\"item_2\",\"type\":\"command_execution\",\"command\":\"dir\"}}\n";
    require(throws<CliError>([&] { parse_codex_events(tool); }), "Any tool use invalidates the answer");
    require(throws<CliError>([] {
        parse_codex_events("{\"type\":\"turn.failed\",\"error\":{\"message\":\"usage limit\"}}\n");
    }), "A failed turn is an error");
    require(throws<CliError>([] {
        parse_codex_events("{\"type\":\"turn.completed\",\"usage\":{\"input_tokens\":1,\"output_tokens\":1}}\n");
    }), "No answer is an error");
}
void bridge_answers_in_chat_completions_shape() {
    const auto settings = parse_settings(settings_json);
    FakeRunner runner({0, claude_success, false});
    Bridge bridge(settings, runner);
    const auto response = bridge.handle(chat("claude-opus-5-5/high", "What is 17 + 25?"));
    require(response.status == 200, "Success");
    require(runner.calls.size() == 1 && runner.calls[0].input == "What is 17 + 25?", "The prompt goes to stdin");
    require(runner.calls[0].working_directory == "C:/bridge/empty" && runner.calls[0].timeout_ms == 1000,
        "Runs in the empty folder with the timeout");
    const auto body = Json::parse(response.body);
    require(body["model"] == "claude-opus-5-5/high", "The profile ID is the reported model");
    require(body["choices"][0]["message"]["content"] == "42" && body["choices"][0]["finish_reason"] == "stop", "Answer");
    require(body["usage"]["prompt_tokens"] == 739 && body["usage"]["completion_tokens"] == 3, "Usage");
}
void bridge_refuses_bad_requests() {
    const auto settings = parse_settings(settings_json);
    FakeRunner runner({0, claude_success, false});
    Bridge bridge(settings, runner);
    require(bridge.handle(chat("unknown", "q")).status == 400, "Unknown model");
    require(bridge.handle("{").status == 400, "Malformed JSON");
    auto two = Json::parse(chat("claude-opus-5-5/high", "q"));
    two["messages"].push_back({{"role", "user"}, {"content", "again"}});
    require(bridge.handle(two.dump()).status == 400, "One user message only");
    auto streamed = Json::parse(chat("claude-opus-5-5/high", "q"));
    streamed["stream"] = true;
    require(bridge.handle(streamed.dump()).status == 400, "No streaming");
    require(runner.calls.empty(), "Refused requests start nothing");
}
void bridge_preserves_system_message() {
    const auto settings = parse_settings(settings_json);
    FakeRunner runner({0, codex_success, false});
    Bridge bridge(settings, runner);
    auto request = Json::parse(chat("gpt-6.1-sol/low", "Find the correction"));
    request["messages"] = Json::array({{{"role", "system"}, {"content", "Return JSON only"}},
        {{"role", "user"}, {"content", "Find the correction"}}});
    require(bridge.handle(request.dump()).status == 200, "A leading system message is accepted");
    require(runner.calls.size() == 1 && runner.calls[0].system_prompt == "Return JSON only" &&
        runner.calls[0].input == "Find the correction", "System and user content remain separate");
    require(bridge.metrics().system_prompt_sha256 == std::vector<std::string>{ltm::sha256("Return JSON only")},
        "The actual system instruction is fingerprinted without storing its text");
    FakeRunner other({0, claude_success, false});
    Bridge other_bridge(settings, other);
    request["model"] = "claude-opus-5-5/high";
    require(other_bridge.handle(request.dump()).status == 200 &&
        contains(other.calls[0].command, {"--system-prompt", "Return JSON only"}),
        "The leading instruction reaches the requested CLI system setting");
    request["messages"][0]["role"] = "assistant";
    require(other_bridge.handle(request.dump()).status == 400 && other.calls.size() == 1,
        "Unexpected message sequences are refused");
}
void bridge_limits_cli_attempts() {
    auto config = Json::parse(settings_json);
    config["max_calls"] = 2;
    config["retries"] = 2;
    FakeRunner runner(std::vector<RunResult>{{1, "fail", false}, {0, codex_success, false}});
    Bridge bridge(parse_settings(config.dump()), runner);
    require(bridge.handle(chat("gpt-6.1-sol/low", "q")).status == 200, "Retry can use second permitted call");
    require(runner.calls.size() == 2, "Retries count toward call limit");
    require(bridge.handle(chat("gpt-6.1-sol/low", "q")).status == 429, "Exhausted limit is refused");
    require(runner.calls.size() == 2, "Exhausted limit starts no process");
}
void bridge_reports_usage_and_attempts() {
    auto config = Json::parse(settings_json);
    config["retries"] = 1;
    FakeRunner runner(std::vector<RunResult>{{1, "failed", false}, {0, codex_success, false}});
    Bridge bridge(parse_settings(config.dump()), runner);
    require(bridge.handle(chat("gpt-6.1-sol/low", "q")).status == 200, "Request succeeds after retry");
    const auto metrics = bridge.metrics();
    require(metrics.attempts == 2 && metrics.completed == 1 && metrics.input_tokens == 6547 &&
        metrics.output_tokens == 5, "Usage records successful CLI output and counts every attempt");
    const std::string no_usage =
        "{\"type\":\"item.completed\",\"item\":{\"type\":\"agent_message\",\"text\":\"42\"}}\n"
        "{\"type\":\"turn.completed\"}\n";
    FakeRunner missing({0, no_usage, false});
    Bridge missing_bridge(parse_settings(settings_json), missing);
    require(missing_bridge.handle(chat("gpt-6.1-sol/low", "q")).status == 200 &&
        missing_bridge.metrics().missing_usage == 1, "An answer without token counts is identified");
}
void bridge_call_limit_is_atomic() {
    class CountingRunner : public CommandRunner {
      public:
        std::atomic<unsigned> calls{0};
        RunResult run(const Invocation&) override { ++calls; return {0, codex_success, false}; }
    } runner;
    auto config = Json::parse(settings_json);
    config["max_calls"] = 5;
    Bridge bridge(parse_settings(config.dump()), runner);
    std::atomic<unsigned> allowed{0}, refused{0};
    std::vector<std::thread> threads;
    for (int i = 0; i < 20; ++i) threads.emplace_back([&] {
        const auto status = bridge.handle(chat("gpt-6.1-sol/low", "q")).status;
        if (status == 200) ++allowed;
        if (status == 429) ++refused;
    });
    for (auto& thread : threads) thread.join();
    require(allowed == 5 && refused == 15 && runner.calls == 5, "Concurrent requests cannot exceed cap");
}
void bridge_limits_concurrent_runs() {
    class ConcurrentRunner : public CommandRunner {
      public:
        std::atomic<unsigned> active{0}, peak{0};
        RunResult run(const Invocation&) override {
            const auto now = ++active;
            auto observed = peak.load();
            while (now > observed && !peak.compare_exchange_weak(observed, now)) {}
            std::this_thread::sleep_for(std::chrono::milliseconds(40));
            --active;
            return {0, codex_success, false};
        }
    } runner;
    auto config = Json::parse(settings_json);
    config["max_concurrent"] = 1;
    Bridge bridge(parse_settings(config.dump()), runner);
    std::atomic<unsigned> completed{0};
    std::vector<std::thread> threads;
    for (int i = 0; i < 6; ++i) threads.emplace_back([&] {
        if (bridge.handle(chat("gpt-6.1-sol/low", "q")).status == 200) ++completed;
    });
    for (auto& thread : threads) thread.join();
    require(completed == 6 && runner.peak == 1, "Concurrent requests wait for an available slot");
}
void bridge_reports_cli_failures() {
    const auto settings = parse_settings(settings_json);
    FakeRunner timed_out({-1, "", true});
    require(Bridge(settings, timed_out).handle(chat("claude-opus-5-5/high", "q")).status == 504, "Timeout");
    FakeRunner failed({1, "", false});
    require(Bridge(settings, failed).handle(chat("claude-opus-5-5/high", "q")).status == 502, "Nonzero exit");
    auto other = Json::parse(claude_success);
    other["modelUsage"] = {{"claude-sonnet-5-5", Json::object()}};
    FakeRunner wrong_model({0, other.dump(), false});
    const auto response = Bridge(settings, wrong_model).handle(chat("claude-opus-5-5/high", "q"));
    require(response.status == 502 && Json::parse(response.body)["error"]["code"] == "model_mismatch",
        "A different served model is refused");
    FakeRunner codex({0, codex_success, false});
    Bridge codex_bridge(settings, codex);
    require(codex_bridge.handle(chat("gpt-6.1-sol/low", "q")).status == 200 &&
        codex.calls[0].command[0] == "C:/tools/codex.exe", "Codex profiles run Codex");
}
void bridge_retries_failed_runs_and_logs_them() {
    auto json = Json::parse(settings_json);
    json["retries"] = 2;
    json["retry_delay_ms"] = 0;
    const auto settings = parse_settings(json.dump());
    std::vector<std::string> logged;
    const auto log = [&](const Profile& profile, const std::string& code, const RunResult& run) {
        logged.push_back(profile.id + " " + code + " " + run.output);
    };
    FakeRunner flaky(std::vector<RunResult>{{1, "boom", false}, {0, "{\"type\":\"turn.failed\"}", false},
        {0, codex_success, false}});
    Bridge bridge(settings, flaky);
    bridge.on_failure(log);
    require(bridge.handle(chat("gpt-6.1-sol/low", "q")).status == 200 && flaky.calls.size() == 3,
        "A failed run is retried until it answers");
    require(logged.size() == 2 && logged[0] == "gpt-6.1-sol/low cli_exit_1 boom" &&
        logged[1].rfind("gpt-6.1-sol/low cli_error", 0) == 0, "Each failure is logged with the CLI output");
    FakeRunner broken({1, "", false});
    require(Bridge(settings, broken).handle(chat("gpt-6.1-sol/low", "q")).status == 502 && broken.calls.size() == 3,
        "Retries are bounded");
    FakeRunner slow({-1, "", true});
    require(Bridge(settings, slow).handle(chat("gpt-6.1-sol/low", "q")).status == 504 && slow.calls.size() == 1,
        "A timeout is not retried");
    FakeRunner once({1, "", false});
    require(Bridge(parse_settings(settings_json), once).handle(chat("gpt-6.1-sol/low", "q")).status == 502 &&
        once.calls.size() == 1, "No retries unless configured");
}
}

int main(int argc, char** argv) {
    const std::vector<std::pair<std::string, std::function<void()>>> cases = {
        {"settings_name_profiles", settings_name_profiles},
        {"claude_runs_without_tools_or_settings", claude_runs_without_tools_or_settings},
        {"codex_runs_without_tools_or_user_config", codex_runs_without_tools_or_user_config},
        {"claude_result_is_parsed", claude_result_is_parsed}, {"codex_events_are_parsed", codex_events_are_parsed},
        {"bridge_answers_in_chat_completions_shape", bridge_answers_in_chat_completions_shape},
        {"bridge_refuses_bad_requests", bridge_refuses_bad_requests},
        {"bridge_preserves_system_message", bridge_preserves_system_message},
        {"bridge_limits_cli_attempts", bridge_limits_cli_attempts},
        {"bridge_reports_usage_and_attempts", bridge_reports_usage_and_attempts},
        {"bridge_call_limit_is_atomic", bridge_call_limit_is_atomic},
        {"bridge_limits_concurrent_runs", bridge_limits_concurrent_runs},
        {"bridge_reports_cli_failures", bridge_reports_cli_failures},
        {"bridge_retries_failed_runs_and_logs_them", bridge_retries_failed_runs_and_logs_them}};
    if (argc != 2) return 2;
    for (const auto& item : cases) {
        if (item.first == argv[1]) {
            try { item.second(); std::cout << "PASS " << item.first << '\n'; return 0; }
            catch (const std::exception& e) { std::cerr << "FAIL " << item.first << ": " << e.what() << '\n'; return 1; }
        }
    }
    return 2;
}
