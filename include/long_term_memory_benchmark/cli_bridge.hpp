#pragma once

#include <atomic>
#include <condition_variable>
#include <cstdint>
#include <functional>
#include <mutex>
#include <stdexcept>
#include <string>
#include <string_view>
#include <vector>

// Serves OpenAI-shaped chat completions on loopback by running the owner's own
// signed-in Claude Code or Codex CLI once per request, with tools disabled and a
// neutral system prompt. Benchmark use on the owner's machine only: each CLI
// authenticates itself and usage counts against the owner's plan.
namespace ltm::cli_bridge {
enum class Cli { claude, codex };

struct Profile {
    std::string id;      // the model name clients send and get back
    Cli cli = Cli::claude;
    std::string model;   // the model the CLI is asked for
    std::string effort;  // Claude Code --effort, Codex model_reasoning_effort
};

struct Settings {
    std::string claude_executable;
    std::string codex_executable;
    std::string system_prompt;
    std::string codex_instructions_file;  // holds system_prompt for Codex
    std::string working_directory;        // an empty folder
    std::uint32_t timeout_ms = 0;
    // A run that ends without an answer is tried again; a subscription CLI has no
    // per-request charge. Timeouts are not retried.
    std::uint64_t max_calls = 0;  // zero leaves call count unrestricted
    std::uint32_t max_concurrent = 0;  // zero leaves concurrency unrestricted
    std::uint32_t retries = 0;
    std::uint32_t retry_delay_ms = 0;  // multiplied by the attempt number
    std::vector<Profile> profiles;
};
Settings parse_settings(const std::string& json);
const Profile* find_profile(const Settings& settings, const std::string& id);
std::vector<std::string> command(const Settings& settings, const Profile& profile);

struct Reply {
    std::string text;
    std::string model;  // as served, when the CLI reports it
    std::uint64_t input_tokens = 0;
    std::uint64_t output_tokens = 0;
};
// Messages are short codes, safe to return to clients and keep in artifacts.
class CliError : public std::runtime_error {
  public:
    using std::runtime_error::runtime_error;
};
Reply parse_claude_result(std::string_view output);
Reply parse_codex_events(std::string_view output);

struct Invocation {
    std::vector<std::string> command;
    std::string input;
    std::string working_directory;
    std::uint32_t timeout_ms = 0;
};
struct RunResult {
    int exit_code = 0;
    std::string output;  // standard output only
    bool timed_out = false;
};
class CommandRunner {
  public:
    virtual ~CommandRunner() = default;
    virtual RunResult run(const Invocation& invocation) = 0;
};

struct Response {
    unsigned status = 0;
    std::string body;
};
// Called for each failed run with the error code and the CLI's output.
using FailureLog = std::function<void(const Profile&, const std::string& code, const RunResult&)>;

// Handles one POST /v1/chat/completions body. Safe to call from several threads
// if the failure log is.
class Bridge {
  public:
    Bridge(Settings settings, CommandRunner& runner);
    void on_failure(FailureLog log);
    Response handle(const std::string& body);

  private:
    Settings settings_;
    CommandRunner& runner_;
    std::atomic<std::uint64_t> calls_{0};
    std::mutex slots_mutex_;
    std::condition_variable slots_available_;
    std::uint32_t active_ = 0;
    FailureLog log_ = [](const Profile&, const std::string&, const RunResult&) {};
};
} // namespace ltm::cli_bridge
