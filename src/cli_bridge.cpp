#include "long_term_memory_benchmark/cli_bridge.hpp"
#include <nlohmann/json.hpp>
#include <atomic>
#include <chrono>
#include <sstream>
#include <thread>
#include <utility>

namespace ltm::cli_bridge {
namespace {
using Json = nlohmann::json;

// Codex features that give the model tools: commands, code, browsers, apps,
// agents, images. Disabled so the reader can only read its prompt.
const char* const codex_tool_features[] = {"shell_tool", "unified_exec", "code_mode_host", "apps", "plugins",
    "browser_use", "browser_use_external", "in_app_browser", "computer_use", "goals", "multi_agent",
    "collaboration_modes", "image_generation", "sleep_tool", "view_image", "hooks", "skill_search",
    "tool_suggest", "remote_plugin"};
// Codex event items that are not tool use. An "error" item is a notice, such as
// code mode being unavailable because it is disabled; a failed turn is an event.
bool answer_item(const std::string& type) {
    return type == "agent_message" || type == "reasoning" || type == "error";
}

std::string text_field(const Json& object, const char* name) {
    if (!object.contains(name) || !object.at(name).is_string() || object.at(name).get<std::string>().empty())
        throw std::invalid_argument(std::string("Missing text field: ") + name);
    return object.at(name).get<std::string>();
}
std::uint64_t count(const Json& object, const char* name) {
    return object.contains(name) && object.at(name).is_number_unsigned() ? object.at(name).get<std::uint64_t>() : 0;
}
Json parse_or_fail(std::string_view text) {
    auto json = Json::parse(text.begin(), text.end(), nullptr, false);
    if (json.is_discarded() || !json.is_object()) throw CliError("unreadable_output");
    return json;
}
Response error(unsigned status, const std::string& code) {
    return {status, Json{{"error", {{"code", code}}}}.dump()};
}
}

Settings parse_settings(const std::string& json) {
    const auto root = Json::parse(json, nullptr, false);
    if (root.is_discarded() || !root.is_object()) throw std::invalid_argument("Settings are not a JSON object");
    Settings settings;
    settings.claude_executable = text_field(root, "claude_executable");
    settings.codex_executable = text_field(root, "codex_executable");
    settings.system_prompt = text_field(root, "system_prompt");
    settings.codex_instructions_file = text_field(root, "codex_instructions_file");
    settings.working_directory = text_field(root, "working_directory");
    if (!root.contains("timeout_ms") || !root.at("timeout_ms").is_number_unsigned() ||
        root.at("timeout_ms").get<std::uint64_t>() == 0 || root.at("timeout_ms").get<std::uint64_t>() > UINT32_MAX)
        throw std::invalid_argument("timeout_ms must be a positive integer");
    settings.timeout_ms = root.at("timeout_ms").get<std::uint32_t>();
    for (auto [name, field] : {std::pair{"retries", &settings.retries}, std::pair{"retry_delay_ms", &settings.retry_delay_ms}}) {
        if (!root.contains(name)) continue;
        if (!root.at(name).is_number_unsigned() || root.at(name).get<std::uint64_t>() > 600000)
            throw std::invalid_argument(std::string(name) + " must be a small nonnegative integer");
        *field = root.at(name).get<std::uint32_t>();
    }
    if (root.contains("max_calls")) {
        if (!root.at("max_calls").is_number_unsigned()) throw std::invalid_argument("max_calls must be nonnegative");
        settings.max_calls = root.at("max_calls").get<std::uint64_t>();
    }
    if (!root.contains("profiles") || !root.at("profiles").is_array() || root.at("profiles").empty())
        throw std::invalid_argument("profiles must list at least one profile");
    for (const auto& item : root.at("profiles")) {
        Profile profile{text_field(item, "id"), Cli::claude, text_field(item, "model"), text_field(item, "effort")};
        const auto cli = text_field(item, "cli");
        if (cli == "codex") profile.cli = Cli::codex;
        else if (cli != "claude") throw std::invalid_argument("Unknown CLI: " + cli);
        if (find_profile(settings, profile.id)) throw std::invalid_argument("Duplicate profile: " + profile.id);
        settings.profiles.push_back(std::move(profile));
    }
    return settings;
}

const Profile* find_profile(const Settings& settings, const std::string& id) {
    for (const auto& profile : settings.profiles)
        if (profile.id == id) return &profile;
    return nullptr;
}

std::vector<std::string> command(const Settings& settings, const Profile& profile) {
    if (profile.cli == Cli::claude) {
        return {settings.claude_executable, "-p", "--model", profile.model, "--effort", profile.effort,
            "--tools", "", "--strict-mcp-config", "--setting-sources", "", "--no-session-persistence",
            "--disable-slash-commands", "--system-prompt", settings.system_prompt, "--output-format", "json"};
    }
    std::vector<std::string> args{settings.codex_executable, "exec", "--ignore-user-config", "--ephemeral",
        "--skip-git-repo-check", "-s", "read-only", "-m", profile.model,
        "-c", "model_reasoning_effort=\"" + profile.effort + "\"",
        "-c", "model_instructions_file='" + settings.codex_instructions_file + "'",
        "-c", "web_search=\"disabled\""};
    for (const auto* feature : codex_tool_features) {
        args.push_back("--disable");
        args.push_back(feature);
    }
    args.push_back("--json");
    args.push_back("-");
    return args;
}

Reply parse_claude_result(std::string_view output) {
    const auto result = parse_or_fail(output);
    if (result.value("type", "") != "result") throw CliError("unreadable_output");
    if (result.value("is_error", true) || result.value("subtype", "") != "success")
        throw CliError("cli_error");
    if (!result.contains("result") || !result.at("result").is_string()) throw CliError("no_answer");
    if (!result.contains("modelUsage") || !result.at("modelUsage").is_object() || result.at("modelUsage").size() != 1)
        throw CliError("model_unclear");
    Reply reply;
    reply.text = result.at("result").get<std::string>();
    reply.model = result.at("modelUsage").begin().key();
    const auto& usage = result.value("usage", Json::object());
    reply.input_tokens = count(usage, "input_tokens") + count(usage, "cache_creation_input_tokens") +
        count(usage, "cache_read_input_tokens");
    reply.output_tokens = count(usage, "output_tokens");
    if (reply.text.empty()) throw CliError("no_answer");
    return reply;
}

Reply parse_codex_events(std::string_view output) {
    Reply reply;
    bool completed = false;
    std::istringstream lines{std::string(output)};
    for (std::string line; std::getline(lines, line);) {
        if (!line.empty() && line.back() == '\r') line.pop_back();
        if (line.empty()) continue;
        const auto event = parse_or_fail(line);
        const auto type = event.value("type", "");
        // Top-level "error" events also report recovered transport retries, so only
        // a failed turn (or no completed answer, below) counts as failure.
        if (type == "turn.failed") throw CliError("cli_error");
        if (type.rfind("item.", 0) == 0) {
            const auto& item = event.value("item", Json::object());
            const auto item_type = item.value("type", "");
            if (!answer_item(item_type)) throw CliError("tool_use");
            if (type == "item.completed" && item_type == "agent_message") reply.text = item.value("text", "");
        }
        if (type == "turn.completed") {
            const auto& usage = event.value("usage", Json::object());
            reply.input_tokens = count(usage, "input_tokens");
            reply.output_tokens = count(usage, "output_tokens");
            completed = true;
        }
    }
    if (!completed || reply.text.empty()) throw CliError("no_answer");
    return reply;
}

Bridge::Bridge(Settings settings, CommandRunner& runner) : settings_(std::move(settings)), runner_(runner) {}
void Bridge::on_failure(FailureLog log) { log_ = std::move(log); }

Response Bridge::handle(const std::string& body) {
    const auto request = Json::parse(body, nullptr, false);
    if (request.is_discarded() || !request.is_object()) return error(400, "malformed_request");
    const auto* profile = request.contains("model") && request.at("model").is_string()
        ? find_profile(settings_, request.at("model").get<std::string>()) : nullptr;
    if (!profile) return error(400, "unknown_model");
    if (request.value("stream", false)) return error(400, "streaming_unsupported");
    const auto& messages = request.value("messages", Json::array());
    if (messages.size() != 1 || !messages[0].is_object() || messages[0].value("role", "") != "user" ||
        !messages[0].contains("content") || !messages[0].at("content").is_string())
        return error(400, "one_user_message_required");

    const Invocation invocation{command(settings_, *profile), messages[0].at("content").get<std::string>(),
        settings_.working_directory, settings_.timeout_ms};
    Reply reply;
    for (std::uint32_t attempt = 0;; ++attempt) {
        if (attempt) std::this_thread::sleep_for(std::chrono::milliseconds(settings_.retry_delay_ms) * attempt);
        auto current = calls_.load();
        do {
            if (settings_.max_calls && current >= settings_.max_calls) return error(429, "call_limit_reached");
        } while (!calls_.compare_exchange_weak(current, current + 1));
        const auto run = runner_.run(invocation);
        if (run.timed_out) {
            log_(*profile, "cli_timeout", run);
            return error(504, "cli_timeout");
        }
        std::string code;
        if (run.exit_code != 0) {
            code = "cli_exit_" + std::to_string(run.exit_code);
        } else {
            try {
                reply = profile->cli == Cli::claude ? parse_claude_result(run.output) : parse_codex_events(run.output);
            } catch (const CliError& failure) {
                code = failure.what();
            }
        }
        if (code.empty() && !reply.model.empty() && reply.model != profile->model) {
            log_(*profile, "model_mismatch", run);
            return error(502, "model_mismatch");
        }
        if (code.empty()) break;
        log_(*profile, code, run);
        if (attempt >= settings_.retries) return error(502, code);
    }

    static std::atomic<std::uint64_t> sequence{0};
    const Json response{{"id", "cli-bridge-" + std::to_string(++sequence)}, {"object", "chat.completion"},
        {"model", profile->id},
        {"choices", {{{"index", 0}, {"message", {{"role", "assistant"}, {"content", reply.text}}},
            {"finish_reason", "stop"}}}},
        {"usage", {{"prompt_tokens", reply.input_tokens}, {"completion_tokens", reply.output_tokens},
            {"total_tokens", reply.input_tokens + reply.output_tokens}}}};
    return {200, response.dump()};
}
} // namespace ltm::cli_bridge
