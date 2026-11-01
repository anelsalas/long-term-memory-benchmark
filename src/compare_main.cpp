#include "long_term_memory_benchmark/experiment.hpp"
#include "long_term_memory_benchmark/fresh_facts.hpp"
#include "long_term_memory_benchmark/hash.hpp"
#include "long_term_memory_benchmark/http_provider.hpp"
#include "long_term_memory_benchmark/memory.hpp"
#include "long_term_memory_benchmark/resources.hpp"
#include "long_term_memory_benchmark/scoring.hpp"
#include <algorithm>
#include <atomic>
#include <chrono>
#include <cstdlib>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <limits>
#include <nlohmann/json.hpp>
#include <set>
#include <stdexcept>
#include <string>
#define NOMINMAX
#include <windows.h>

namespace {
using Json = nlohmann::json;
namespace fs = std::filesystem;
std::atomic<bool> cancelled{false};
BOOL WINAPI cancel_handler(DWORD event) {
    if (event == CTRL_C_EVENT || event == CTRL_BREAK_EVENT) {
        cancelled.store(true);
        return TRUE;
    }
    return FALSE;
}
std::uint64_t integer(const Json& value) {
    if (value.is_number_unsigned()) {
        return value.get<std::uint64_t>();
    }
    if (value.is_number_integer() && value.get<std::int64_t>() >= 0) {
        return static_cast<std::uint64_t>(value.get<std::int64_t>());
    }
    if (value.is_string()) {
        const auto text = value.get<std::string>();
        if (text.empty() ||
            std::any_of(text.begin(), text.end(), [](char c) { return c < '0' || c > '9'; })) {
            throw std::invalid_argument("Expected unsigned integer");
        }
        std::size_t count = 0;
        const auto result = std::stoull(text, &count);
        if (count == text.size()) {
            return result;
        }
    }
    throw std::invalid_argument("Expected unsigned integer");
}
std::string read_bounded(const fs::path& path, std::uintmax_t limit) {
    const auto size = fs::file_size(path);
    if (size > limit) {
        throw std::invalid_argument("Input file exceeds declared size cap");
    }
    std::ifstream input(path, std::ios::binary);
    if (!input) {
        throw std::invalid_argument("Input file unavailable");
    }
    std::string content(static_cast<std::size_t>(size), '\0');
    if (size && !input.read(content.data(), static_cast<std::streamsize>(size))) {
        throw std::invalid_argument("Input file read incomplete");
    }
    if (input.peek() != std::char_traits<char>::eof()) {
        throw std::invalid_argument("Input file changed while reading");
    }
    return content;
}
Json parse_strict(const std::string& text) {
    std::vector<std::set<std::string>> keys;
    return Json::parse(text, [&](int, Json::parse_event_t event, Json& value) {
        if (event == Json::parse_event_t::object_start) {
            keys.emplace_back();
        } else if (event == Json::parse_event_t::key &&
                   !keys.back().insert(value.get<std::string>()).second) {
            throw std::invalid_argument("Duplicate JSON key");
        } else if (event == Json::parse_event_t::object_end) {
            keys.pop_back();
        }
        return true;
    });
}
void known_fields(const Json& value, const std::set<std::string>& names) {
    if (!value.is_object()) {
        throw std::invalid_argument("Expected configuration object");
    }
    for (auto item = value.begin(); item != value.end(); ++item) {
        if (!names.count(item.key())) {
            throw std::invalid_argument("Unknown configuration field");
        }
    }
}
ltm::ProviderPolicy policy_from(const Json& value) {
    known_fields(value, {"max_requests", "max_tokens", "max_cost_nanos", "input_token_price_nanos",
                         "output_token_price_nanos", "request_price_nanos", "max_prompt_bytes",
                         "max_response_bytes", "max_attempts"});
    ltm::ProviderPolicy policy;
    policy.max_requests = integer(value.at("max_requests"));
    policy.max_tokens = integer(value.at("max_tokens"));
    policy.max_cost_nanos = integer(value.at("max_cost_nanos"));
    policy.input_token_price_nanos = integer(value.at("input_token_price_nanos"));
    policy.output_token_price_nanos = integer(value.at("output_token_price_nanos"));
    policy.request_price_nanos = integer(value.at("request_price_nanos"));
    policy.max_prompt_bytes = integer(value.at("max_prompt_bytes"));
    policy.max_response_bytes = integer(value.at("max_response_bytes"));
    const auto attempts = integer(value.at("max_attempts"));
    if (!attempts || attempts > 100) {
        throw std::invalid_argument("Attempt limit must be 1 to 100");
    }
    policy.max_attempts = static_cast<std::uint32_t>(attempts);
    return policy;
}
struct WorkCase {
    std::string name;
    ltm::LoadedLongMemEvalCase data;
    std::optional<int> fresh_score;
};
std::string timestamp(const std::string& date) {
    if (date.size() == 20 && date[10] == 'T') {
        return date;
    }
    if (date.size() >= 16 && date[4] == '/' && date[7] == '/') {
        auto day = date.substr(0, 10);
        day[4] = '-';
        day[7] = '-';
        return day + "T" + date.substr(date.size() - 5) + ":00Z";
    }
    throw std::invalid_argument("Unsupported history timestamp format");
}
ltm::CandidateCase select_evidence(const ltm::CandidateCase& input, std::size_t top_k,
                                   bool full_history, std::vector<std::string>& selected) {
    if (full_history) {
        for (const auto& session : input.sessions) {
            for (const auto& message : session.messages) {
                selected.push_back(message.source_id);
            }
        }
        return input;
    }
    ltm::LexicalMemory memory;
    for (const auto& session : input.sessions) {
        for (const auto& message : session.messages) {
            memory.store({message.source_id, "case", timestamp(session.date), message.content});
        }
    }
    for (const auto& match : memory.search({input.question, "case", top_k, ""})) {
        selected.push_back(match.record.id);
    }
    const std::set<std::string> allowed(selected.begin(), selected.end());
    auto output = input;
    output.sessions.clear();
    for (const auto& session : input.sessions) {
        auto included = session;
        included.messages.clear();
        for (const auto& message : session.messages) {
            if (allowed.count(message.source_id)) {
                included.messages.push_back(message);
            }
        }
        if (!included.messages.empty()) {
            output.sessions.push_back(std::move(included));
        }
    }
    return output;
}
const std::string answer_instruction =
    "Answer the question from the supplied timestamped history. Use the latest applicable facts. "
    "If the information is absent, say that you do not know. Give the shortest complete "
    "answer.\n\n";
std::string compact_local_prompt(const ltm::CandidateCase& candidate) {
    std::string prompt = "Use the latest recorded score. Reply only with its integer.\n";
    for (const auto& session : candidate.sessions) {
        for (const auto& message : session.messages) {
            prompt += session.date + " " + message.content + "\n";
        }
    }
    return prompt + candidate.question;
}
Json usage_json(const std::optional<ltm::ProviderUsage>& usage) {
    if (!usage) {
        return nullptr;
    }
    return {{"input_tokens", usage->input_tokens}, {"output_tokens", usage->output_tokens}};
}
Json attempt_json(const ltm::ProviderOutcome& outcome) {
    Json attempts = Json::array();
    for (const auto& attempt : outcome.attempts) {
        attempts.push_back({{"dispatch_id", attempt.dispatch_id},
                            {"prompt_sha256", attempt.prompt_sha256},
                            {"prompt", attempt.request.prompt},
                            {"output_token_limit", attempt.request.output_token_limit},
                            {"provider_request_id", attempt.reply.request_id},
                            {"reply", attempt.reply.text},
                            {"error", attempt.reply.error},
                            {"provider_status", static_cast<int>(attempt.reply.status)},
                            {"usage", usage_json(attempt.reply.usage)},
                            {"reserved_tokens", attempt.reserved_tokens},
                            {"reserved_cost_nanos", attempt.reserved_cost_nanos},
                            {"completed_after_deadline", attempt.completed_after_deadline}});
    }
    return {{"configuration_hash", outcome.configuration_hash},
            {"lifecycle_configuration_hash", outcome.lifecycle_configuration_hash},
            {"attempts", attempts}};
}
bool redact(Json& value, const std::string& secret) {
    if (secret.empty()) {
        return false;
    }
    bool changed = false;
    if (value.is_string()) {
        auto text = value.get<std::string>();
        std::size_t position = 0;
        while ((position = text.find(secret, position)) != std::string::npos) {
            text.replace(position, secret.size(), "[REDACTED]");
            position += 10;
            changed = true;
        }
        if (changed) {
            value = text;
        }
    } else if (value.is_structured()) {
        for (auto& child : value) {
            changed = redact(child, secret) || changed;
        }
    }
    return changed;
}
void write_artifact(const fs::path& directory, const std::string& name, Json value,
                    const std::string& secret = "") {
    if (redact(value, secret) && value.is_object()) {
        value["redaction_applied"] = true;
    }
    const auto text = value.dump(2);
    const auto path = directory / name;
    if (fs::exists(path)) {
        throw std::invalid_argument("Artifact already exists");
    }
    std::ofstream output(path, std::ios::binary);
    if (!output || !output.write(text.data(), static_cast<std::streamsize>(text.size()))) {
        throw std::runtime_error("Artifact write failed");
    }
    output.flush();
    if (!output) {
        throw std::runtime_error("Artifact flush failed");
    }
}
fs::path new_run_directory(const fs::path& root) {
    fs::create_directories(root);
    const auto stamp = std::chrono::system_clock::now().time_since_epoch().count();
    for (unsigned suffix = 0; suffix < 1000; ++suffix) {
        const auto directory =
            root / ("run-" + std::to_string(stamp) + "-" + std::to_string(suffix));
        if (fs::create_directory(directory)) {
            return directory;
        }
    }
    throw std::runtime_error("Unique run directory could not be created");
}
void check_prompt_bound(const std::string& prompt, std::uint64_t bound, std::uint64_t framing) {
    if (framing > bound || prompt.size() > bound - framing) {
        throw std::invalid_argument(
            "Prompt byte bound plus framing exceeds configured input token upper bound");
    }
}
std::string credential_from_environment(const std::string& name) {
    if (name.empty() || std::any_of(name.begin(), name.end(), [](unsigned char c) {
            return !(c == '_' || (c >= 'A' && c <= 'Z') || (c >= 'a' && c <= 'z') ||
                     (c >= '0' && c <= '9'));
        })) {
        throw std::invalid_argument("Credential environment variable name is invalid");
    }
    char* value = nullptr;
    std::size_t size = 0;
    if (_dupenv_s(&value, &size, name.c_str()) != 0 || !value || !size) {
        std::free(value);
        throw std::invalid_argument("Configured environment credential unavailable");
    }
    std::string secret(value);
    std::free(value);
    if (secret.empty()) {
        throw std::invalid_argument("Configured environment credential unavailable");
    }
    return secret;
}
} // namespace

int main(int argc, char** argv) {
    if (argc == 2 && std::string(argv[1]) == "--help") {
        std::cout << "ltm-compare --config FILE (--dry-run | --execute)\n";
        return 0;
    }
    if (argc != 4 || std::string(argv[1]) != "--config" ||
        (std::string(argv[3]) != "--dry-run" && std::string(argv[3]) != "--execute")) {
        std::cerr << "Usage: ltm-compare --config FILE (--dry-run | --execute)\n";
        return 2;
    }
    std::string stage = "configuration";
    fs::path artifact_directory;
    try {
        const bool execute = std::string(argv[3]) == "--execute";
        const fs::path config_path = fs::absolute(fs::u8path(argv[2]));
        const auto configuration_bytes = read_bounded(config_path, 1024 * 1024);
        const auto config = parse_strict(configuration_bytes);
        known_fields(config, {"schema_version",
                              "source_revision",
                              "endpoint_url",
                              "api_key_env",
                              "execution_mode",
                              "prompt_profile",
                              "fresh_distractors",
                              "reader",
                              "judge",
                              "provider_policy",
                              "input_token_upper_bound",
                              "context_token_limit",
                              "output_token_limit",
                              "judge_output_token_limit",
                              "seed",
                              "top_k",
                              "timeout_ms",
                              "output_token_parameter",
                              "temperature",
                              "output_directory",
                              "public_case",
                              "chat_framing_token_allowance",
                              "tokenizer_bound_verified",
                              "max_candidate_answer_bytes"});
        if (integer(config.at("schema_version")) != 1) {
            throw std::invalid_argument("Unsupported runtime configuration version");
        }
        const auto policy = policy_from(config.at("provider_policy"));
        const auto execution_mode = config.value("execution_mode", std::string("https"));
        if (execution_mode != "https" && execution_mode != "local") {
            throw std::invalid_argument("Unknown execution mode");
        }
        const bool local_mode = execution_mode == "local";
        const auto prompt_profile = config.value("prompt_profile", std::string("standard"));
        const bool compact_local = prompt_profile == "compact_local_smoke";
        if ((prompt_profile != "standard" && !compact_local) || (compact_local && !local_mode)) {
            throw std::invalid_argument("Unsupported prompt profile for this execution mode");
        }
        const auto distractors =
            config.contains("fresh_distractors") ? integer(config.at("fresh_distractors")) : 8;
        if (distractors > 10000 || (config.contains("fresh_distractors") && !compact_local)) {
            throw std::invalid_argument("Fresh distractor override requires compact local profile");
        }
        if (compact_local && config.contains("public_case") &&
            !config.at("public_case").is_null()) {
            throw std::invalid_argument("Compact local smoke requires only the generated fixture");
        }
        if (local_mode && (policy.max_cost_nanos || policy.input_token_price_nanos ||
                           policy.output_token_price_nanos || policy.request_price_nanos ||
                           (config.contains("api_key_env") && config.at("api_key_env") != ""))) {
            throw std::invalid_argument("Local mode requires zero prices and no credential name");
        }
        const auto input_bound = integer(config.at("input_token_upper_bound"));
        const auto framing = config.contains("chat_framing_token_allowance")
                                 ? integer(config.at("chat_framing_token_allowance"))
                                 : 256;
        const auto answer_byte_limit = config.contains("max_candidate_answer_bytes")
                                           ? integer(config.at("max_candidate_answer_bytes"))
                                           : 65536;
        if (!answer_byte_limit || answer_byte_limit > policy.max_response_bytes) {
            throw std::invalid_argument("Candidate answer byte cap exceeds response cap");
        }
        stage = "execute_budget_gate";
        if (execute && (!policy.max_requests || !policy.max_tokens ||
                        (!local_mode && !policy.max_cost_nanos) ||
                        !config.value("tokenizer_bound_verified", false))) {
            throw std::invalid_argument(
                "Execute requires positive explicit budgets and a verified tokenizer bound");
        }
        stage = "transport_configuration";
        ltm::ChatCompletionsOptions http_options;
        http_options.endpoint_url = config.at("endpoint_url").get<std::string>();
        http_options.local_mode = local_mode;
        http_options.output_token_parameter =
            config.at("output_token_parameter").get<std::string>();
        const auto timeout = integer(config.at("timeout_ms"));
        if (!timeout || timeout > static_cast<std::uint64_t>(std::numeric_limits<int>::max())) {
            throw std::invalid_argument("Timeout outside supported range");
        }
        http_options.timeout_ms = static_cast<std::uint32_t>(timeout);
        if (config.contains("temperature") && !config.at("temperature").is_null()) {
            http_options.temperature = config.at("temperature").get<double>();
        }
        ltm::validate_chat_endpoint(http_options.endpoint_url, local_mode);
        const auto seed = integer(config.at("seed"));
        const auto top_k = integer(config.at("top_k"));
        if (!top_k || top_k > 1000) {
            throw std::invalid_argument("top_k must be 1 to 1000");
        }
        stage = "dataset_preparation";
        const auto ledger =
            ltm::generate_development_ledger(seed, 1, static_cast<std::size_t>(distractors));
        const auto fresh =
            ltm::render_fresh_fact_case(ledger, {"Lina-1", "astronomy", "2026-03-13T10:00:00Z"});
        WorkCase fresh_work;
        fresh_work.name = "fresh";
        fresh_work.data.candidate = fresh.candidate;
        fresh_work.fresh_score = fresh.expected.score;
        fresh_work.data.evaluator.question_id = fresh.candidate.case_id;
        fresh_work.data.evaluator.question_type = "knowledge-update";
        fresh_work.data.evaluator.answer_json = Json(std::to_string(*fresh.expected.score)).dump();
        std::vector<WorkCase> work = {std::move(fresh_work)};
        std::string public_checksum;
        if (config.contains("public_case") && !config.at("public_case").is_null()) {
            const auto& selected = config.at("public_case");
            known_fields(selected, {"path", "question_id", "source_uri", "revision", "sha256"});
            const auto data_path =
                config_path.parent_path() / fs::u8path(selected.at("path").get<std::string>());
            const auto bytes = read_bounded(data_path, 256u * 1024u * 1024u);
            ltm::DatasetProvenance provenance{selected.at("source_uri").get<std::string>(),
                                              selected.at("revision").get<std::string>(),
                                              selected.at("sha256").get<std::string>()};
            auto loaded = ltm::load_longmemeval_case(
                bytes, selected.at("question_id").get<std::string>(), provenance);
            public_checksum = loaded.actual_sha256;
            work.push_back({"public", std::move(loaded), {}});
        }
        ltm::ExperimentManifest manifest;
        manifest.source_revision = config.at("source_revision").get<std::string>();
        manifest.dataset = {
            "fresh-development-plus-optional-LongMemEval", "command-v1",
            ltm::sha256(ltm::candidate_input_json(fresh.candidate) + ":" + public_checksum)};
        for (const auto& item : work) {
            for (const auto* condition : {"full_history", "retrieved_evidence"}) {
                manifest.case_ids.push_back(item.name + ":" + condition);
            }
        }
        manifest.method = "paired-full-vs-ascii-lexical-both-roles";
        manifest.method_revision = "v1";
        manifest.method_parameters = {{"top_k_messages", std::to_string(top_k)}};
        for (const auto* role : {"reader", "judge"}) {
            known_fields(config.at(role), {"id", "version"});
        }
        manifest.reader = {"openai-compatible", config.at("reader").at("id").get<std::string>(),
                           config.at("reader").at("version").get<std::string>()};
        manifest.judge = {"openai-compatible", config.at("judge").at("id").get<std::string>(),
                          config.at("judge").at("version").get<std::string>()};
        manifest.prompt_sha256 = {{"answer_instruction", ltm::sha256(answer_instruction)},
                                  {"runtime_configuration", ltm::sha256(configuration_bytes)}};
        manifest.context_token_limit = integer(config.at("context_token_limit"));
        manifest.output_token_limit = integer(config.at("output_token_limit"));
        manifest.run_parameters = {
            {"seed", std::to_string(seed)},
            {"endpoint", http_options.endpoint_url},
            {"execution_mode", execution_mode},
            {"prompt_profile", prompt_profile},
            {"fresh_distractors", std::to_string(distractors)},
            {"local_finish_reason_limit",
             local_mode ? "service-reported stop is not a verified completion boundary"
                        : "not-applicable"},
            {"judge_temperature", "0"},
            {"transport_revision", "winhttp-chat-v1"},
            {"public_dataset_sha256", public_checksum},
            {"profile", "adapted-comparison"},
            {"candidate_answer_byte_limit", std::to_string(answer_byte_limit)},
            {"original_judge_output_tokens", "10"},
            {"actual_judge_output_tokens",
             std::to_string(integer(config.at("judge_output_token_limit")))}};
        const auto judge_limit = integer(config.at("judge_output_token_limit"));
        if (!judge_limit || judge_limit > manifest.output_token_limit ||
            input_bound > manifest.context_token_limit) {
            throw std::invalid_argument("Judge and input bounds must fit declared limits");
        }
        for (const auto& item : work) {
            if (item.name == "public") {
                const auto preview =
                    ltm::make_official_grade_request(item.data, "0", manifest.judge.version);
                if (answer_byte_limit > input_bound ||
                    preview.prompt.size() > input_bound - answer_byte_limit ||
                    framing > input_bound - answer_byte_limit - preview.prompt.size()) {
                    throw std::invalid_argument(
                        "Judge template and maximum frozen answer exceed input bound");
                }
                if (preview.prompt.size() > policy.max_prompt_bytes ||
                    answer_byte_limit > policy.max_prompt_bytes - preview.prompt.size()) {
                    throw std::invalid_argument("Judge template and answer exceed prompt byte cap");
                }
            }
        }
        stage = "prompt_preflight";
        ltm::ProviderRunner runner(manifest, policy);
        struct Planned {
            const WorkCase* source;
            std::string id, condition, prompt;
            std::vector<std::string> sources;
            double preparation_ms;
        };
        std::vector<Planned> planned;
        Json plans = Json::array();
        std::size_t planned_judges = 0;
        for (const auto& item : work) {
            for (const auto* condition : {"full_history", "retrieved_evidence"}) {
                const auto preparation_start = ltm::ProviderClock::now();
                std::vector<std::string> sources;
                auto candidate =
                    select_evidence(item.data.candidate, static_cast<std::size_t>(top_k),
                                    std::string(condition) == "full_history", sources);
                auto prompt = answer_instruction + ltm::candidate_input_json(candidate);
                if (compact_local) {
                    prompt = compact_local_prompt(candidate);
                    if (prompt.size() + std::string("user").size() + 4 > 512) {
                        throw std::invalid_argument(
                            "Compact local prompt exceeds service context byte cap");
                    }
                } else if (item.name == "fresh") {
                    prompt += "\nFor this score question, respond with only the integer score.";
                }
                check_prompt_bound(prompt, input_bound, framing);
                if (prompt.size() > policy.max_prompt_bytes) {
                    throw std::invalid_argument("Reader prompt exceeds byte cap");
                }
                ltm::ProviderRequest preview;
                preview.model = manifest.reader;
                preview.prompt = prompt;
                preview.output_token_limit = manifest.output_token_limit;
                (void)ltm::chat_completion_body(preview, http_options);
                const auto preparation_ms = std::chrono::duration<double, std::milli>(
                                                ltm::ProviderClock::now() - preparation_start)
                                                .count();
                planned.push_back({&item, item.name + ":" + condition, condition, prompt, sources,
                                   preparation_ms});
                if (item.name == "public") {
                    ++planned_judges;
                }
                plans.push_back({{"case_id", planned.back().id},
                                 {"condition", condition},
                                 {"prompt", prompt},
                                 {"source_ids", sources},
                                 {"prompt_sha256", ltm::sha256(prompt)},
                                 {"prompt_bytes", prompt.size()},
                                 {"input_token_upper_bound", input_bound}});
            }
        }
        const auto root = config_path.parent_path() /
                          fs::u8path(config.at("output_directory").get<std::string>());
        const auto output = new_run_directory(root);
        artifact_directory = output;
        write_artifact(output, "manifest.json", parse_strict(ltm::manifest_to_json(manifest)));
        write_artifact(output, "plan.json",
                       {{"schema_version", 1},
                        {"run_kind", execute ? "execute_plan" : "dry_run_plan"},
                        {"configuration_hash", ltm::configuration_hash(manifest)},
                        {"planned_reader_requests", planned.size()},
                        {"planned_judge_requests", planned_judges},
                        {"provider_requests_made", 0},
                        {"cases", plans}});
        Json gold = Json::array();
        for (const auto& item : work) {
            gold.push_back({{"source", item.name},
                            {"gold_answer", parse_strict(item.data.evaluator.answer_json)},
                            {"question_type", item.data.evaluator.question_type},
                            {"dataset_sha256", item.data.actual_sha256}});
        }
        write_artifact(output, "evaluator-gold.json", gold);
        if (!execute) {
            std::cout << Json{{"schema_version", 1},
                              {"run_kind", "dry_run_plan"},
                              {"artifact_directory", output.u8string()},
                              {"provider_requests_made", 0},
                              {"planned_requests", planned.size() + planned_judges},
                              {"model_accuracy", nullptr}}
                             .dump()
                      << '\n';
            return 0;
        }
        stage = "credential_setup";
        const auto secret =
            local_mode ? std::string{}
                       : credential_from_environment(config.at("api_key_env").get<std::string>());
        ltm::WinHttpTransport transport;
        ltm::ChatCompletionsProvider reader(http_options, secret, transport);
        auto judge_options = http_options;
        judge_options.temperature = 0;
        ltm::ChatCompletionsProvider judge(judge_options, secret, transport);
        SetConsoleCtrlHandler(cancel_handler, TRUE);
        stage = "measured_comparison";
        std::vector<ltm::CaseRecord> records;
        for (std::size_t index = 0; index < planned.size(); ++index) {
            const auto& item = planned[index];
            const auto prefix = "case-" + std::to_string(index);
            write_artifact(
                output, prefix + ".reader-intent.json",
                {{"case_id", item.id}, {"prompt", item.prompt}, {"source_ids", item.sources}},
                secret);
            ltm::RequestControl control;
            control.cancelled = [] { return cancelled.load(); };
            control.deadline =
                ltm::ProviderClock::now() + std::chrono::milliseconds(http_options.timeout_ms);
            const auto start = ltm::ProviderClock::now();
            const auto before = ltm::sample_process_resources();
            const auto answer = runner.run(reader, item.id, ltm::ModelRole::reader, item.prompt,
                                           input_bound, manifest.output_token_limit, control);
            const auto after = ltm::sample_process_resources();
            const auto elapsed =
                std::chrono::duration<double, std::milli>(ltm::ProviderClock::now() - start)
                    .count();
            auto record = answer.record;
            auto result = attempt_json(answer);
            result["reader_wall_ms"] = elapsed;
            result["source_ids"] = item.sources;
            result["retrieval_preparation_ms"] = item.preparation_ms;
            result["response_metadata"] = answer.attempts.empty()
                                              ? Json(nullptr)
                                              : parse_strict(reader.response_metadata_json());
            const auto resource = ltm::resource_interval("reader", before, after, elapsed);
            result["local_resources"] = {
                {"scope", "local_runner_process"},
                {"resident_before_bytes",
                 before.resident_bytes ? Json(*before.resident_bytes) : Json(nullptr)},
                {"resident_after_bytes",
                 after.resident_bytes ? Json(*after.resident_bytes) : Json(nullptr)},
                {"cpu_delta_ms",
                 resource.cpu_delta_ms ? Json(*resource.cpu_delta_ms) : Json(nullptr)},
                {"private_committed_after_bytes", after.private_committed_bytes
                                                      ? Json(*after.private_committed_bytes)
                                                      : Json(nullptr)},
                {"process_lifetime_peak_resident_bytes",
                 after.process_lifetime_peak_resident_bytes
                     ? Json(*after.process_lifetime_peak_resident_bytes)
                     : Json(nullptr)}};
            write_artifact(output, prefix + ".reader-result.json", result, secret);
            if (record.status == ltm::CaseStatus::completed && item.source->fresh_score) {
                auto text = record.answer;
                const auto first = text.find_first_not_of(" \r\n\t");
                const auto last = text.find_last_not_of(" \r\n\t");
                text = first == std::string::npos ? "" : text.substr(first, last - first + 1);
                record.correct = text == std::to_string(*item.source->fresh_score);
                result["fresh_exact_answer_match"] = *record.correct;
                result["grading_profile"] = "independent-ledger-exact-integer-v1";
            }
            if (record.status == ltm::CaseStatus::completed && item.source->name == "public" &&
                record.answer.size() <= answer_byte_limit) {
                const auto grade = ltm::make_official_grade_request(
                    item.source->data, record.answer, manifest.judge.version);
                check_prompt_bound(grade.prompt, input_bound, framing);
                write_artifact(output, prefix + ".judge-intent.json",
                               {{"case_id", item.id},
                                {"prompt", grade.prompt},
                                {"template_sha256", grade.template_sha256},
                                {"frozen_answer_sha256", ltm::sha256(record.answer)}},
                               secret);
                control.deadline =
                    ltm::ProviderClock::now() + std::chrono::milliseconds(http_options.timeout_ms);
                const auto judge_start = ltm::ProviderClock::now();
                const auto graded = runner.run(judge, item.id, ltm::ModelRole::judge, grade.prompt,
                                               input_bound, judge_limit, control);
                result["judge"] = attempt_json(graded);
                result["judge_wall_ms"] = std::chrono::duration<double, std::milli>(
                                              ltm::ProviderClock::now() - judge_start)
                                              .count();
                result["judge_metadata"] = graded.attempts.empty()
                                               ? Json(nullptr)
                                               : parse_strict(judge.response_metadata_json());
                if (graded.record.status == ltm::CaseStatus::completed) {
                    const auto verdict = ltm::parse_judge_verdict(graded.record.answer);
                    if (verdict != ltm::JudgeVerdict::invalid) {
                        record.correct = verdict == ltm::JudgeVerdict::yes;
                    }
                }
            }
            if (item.source->name == "public") {
                result["retrieval_metrics"] = nullptr;
                result["retrieval_metric_status"] =
                    "Full user-only corpus ranking adapter required for official retrieval metrics";
                if (record.answer.size() > answer_byte_limit) {
                    result["grading_error"] = "Frozen answer exceeds declared grading byte cap";
                }
            }
            records.push_back(record);
            write_artifact(output, prefix + ".result.json", result, secret);
        }
        write_artifact(output, "results.json",
                       parse_strict(ltm::results_to_json(manifest, records)), secret);
        const auto summary = ltm::summarize_cases(manifest, records);
        const auto budget = runner.budget();
        const bool complete =
            summary.completed == summary.expected && summary.graded == summary.expected;
        Json report = {{"schema_version", 1},
                       {"run_kind", "measured_comparison"},
                       {"complete", complete},
                       {"artifact_directory", output.u8string()},
                       {"provider_requests_made", budget.dispatched_requests},
                       {"accounted_tokens", budget.accounted_tokens},
                       {"accounted_cost_nanos", budget.accounted_cost_nanos},
                       {"accounting_overflow", budget.accounting_overflow},
                       {"usage_overrun", budget.usage_overrun},
                       {"expected_cases", summary.expected},
                       {"graded_cases", summary.graded},
                       {"graded_accuracy",
                        summary.graded_accuracy ? Json(*summary.graded_accuracy) : Json(nullptr)}};
        write_artifact(output, "summary.json", report, secret);
        std::cout << report.dump() << '\n';
        return complete ? 0 : 3;
    } catch (const std::exception&) {
        std::cerr << "ltm_compare_error:" << stage
                  << ". Verify the declared configuration and access.\n";
        std::cout << Json{{"schema_version", 1},
                          {"run_kind", "command_error"},
                          {"stage", stage},
                          {"artifact_directory", artifact_directory.empty()
                                                     ? Json(nullptr)
                                                     : Json(artifact_directory.u8string())}}
                         .dump()
                  << '\n';
        return 2;
    }
}
