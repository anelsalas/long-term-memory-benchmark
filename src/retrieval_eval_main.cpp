// ltm-retrieval-eval: retrieval baselines on selected LongMemEval_S questions.
#include "long_term_memory_benchmark/embeddings.hpp"
#include "long_term_memory_benchmark/embeddings_http.hpp"
#include "long_term_memory_benchmark/hash.hpp"
#include "long_term_memory_benchmark/retrieval_eval.hpp"
#include <nlohmann/json.hpp>
#include <algorithm>
#include <chrono>
#include <cstdio>
#include <fstream>
#include <iostream>
#include <iterator>
#include <map>
#include <memory>
#include <optional>
#include <set>
#include <sstream>
#include <stdexcept>
#include <string>
#include <vector>

namespace {
using Json = nlohmann::json;
using namespace ltm::retrieval;

const char* usage =
    "Usage: ltm-retrieval-eval --dataset FILE --provenance FILE --questions FILE --out FILE\n"
    "         [--budgets 1000,2000,4000,8000]\n"
    "         [--embeddings-url URL --embedding-model NAME --embedding-cache DIR [--query-prefix TEXT]]\n"
    "  Ranks each question's user rounds with recency and BM25 (and dense and hybrid when an\n"
    "  embeddings URL is given), scores official turn/session retrieval and evidence coverage\n"
    "  per token budget, writes JSON to --out and prints a Markdown table.\n";

std::string read_file(const std::string& path) {
    std::ifstream in(path, std::ios::binary);
    if (!in) throw std::runtime_error("Cannot read " + path);
    return {std::istreambuf_iterator<char>(in), {}};
}
std::vector<std::size_t> parse_budgets(const std::string& text) {
    std::vector<std::size_t> budgets;
    std::stringstream in(text);
    for (std::string item; std::getline(in, item, ',');) budgets.push_back(std::stoul(item));
    return budgets;
}
Json optional_number(const std::optional<double>& value) { return value ? Json(*value) : Json(nullptr); }
std::string percent(const Json& value) {
    if (value.is_null()) return "n/a";
    char text[16];
    std::snprintf(text, sizeof text, "%.1f%%", value.get<double>() * 100);
    return text;
}
Json metrics_json(const ltm::OfficialRetrievalScore& score) {
    if (!score.metrics) return nullptr;
    return {{"recall_any", score.metrics->recall_any}, {"recall_all", score.metrics->recall_all},
        {"ndcg", score.metrics->ndcg}, {"effective_k", score.metrics->effective_k}};
}
Json aggregate_json(const std::vector<std::string>& ids, const std::vector<ltm::OfficialRetrievalScore>& scores) {
    const auto a = ltm::aggregate_official_retrieval(ids, scores);
    return {{"eligible", a.eligible}, {"abstention", a.abstention}, {"no_user_answer_label", a.no_user_answer_label},
        {"missing", a.missing}, {"recall_any", optional_number(a.mean_recall_any)},
        {"recall_all", optional_number(a.mean_recall_all)}, {"ndcg", optional_number(a.mean_ndcg)}};
}
}

int main(int argc, char** argv) {
    std::map<std::string, std::string> options = {{"--budgets", "1000,2000,4000,8000"},
        {"--query-prefix", "Instruct: Given a question, retrieve the conversation turns that answer it\nQuery: "}};
    for (int i = 1; i < argc; ++i) {
        const std::string arg = argv[i];
        if (arg == "--help") { std::cout << usage; return 0; }
        if (arg.rfind("--", 0) != 0 || i + 1 >= argc) { std::cerr << usage; return 2; }
        options[arg] = argv[++i];
    }
    for (const auto* required : {"--dataset", "--provenance", "--questions", "--out"})
        if (!options.count(required)) { std::cerr << usage; return 2; }
    try {
        const auto started = std::chrono::steady_clock::now();
        const auto provenance = Json::parse(read_file(options["--provenance"]));
        const auto question_ids = Json::parse(read_file(options["--questions"])).get<std::vector<std::string>>();
        // Keep only the selected questions in memory: the full file parses to several GB.
        Json dataset;
        {
            auto bytes = read_file(options["--dataset"]);
            if (bytes.size() != provenance.at("bytes").get<std::size_t>() || ltm::sha256(bytes) != provenance.at("sha256"))
                throw std::runtime_error("Dataset size or SHA-256 does not match its provenance");
            const std::set<std::string> wanted(question_ids.begin(), question_ids.end());
            dataset = Json::parse(bytes, [&](int depth, Json::parse_event_t event, Json& parsed) {
                if (depth != 1 || event != Json::parse_event_t::object_end) return true;
                const auto id = parsed.find("question_id");
                return id != parsed.end() && id->is_string() && wanted.count(id->get<std::string>()) > 0;
            });
        }
        std::map<std::string, const Json*> entries;
        for (const auto& entry : dataset) entries[entry.at("question_id").get<std::string>()] = &entry;
        const auto budgets = parse_budgets(options["--budgets"]);

        std::unique_ptr<HttpEmbedder> http;
        std::unique_ptr<CachedEmbedder> cached;
        if (options.count("--embeddings-url")) {
            if (!options.count("--embedding-model") || !options.count("--embedding-cache")) { std::cerr << usage; return 2; }
            http = std::make_unique<HttpEmbedder>(options["--embeddings-url"], options["--embedding-model"]);
            cached = std::make_unique<CachedEmbedder>(*http, options["--embedding-cache"]);
        }
        const auto source_uri = provenance.at("source").get<std::string>() + "/resolve/" +
            provenance.at("revision").get<std::string>() + "/" + provenance.at("name").get<std::string>();

        std::map<std::string, std::vector<ltm::OfficialRetrievalScore>> turn5, turn10, session5;
        std::map<std::string, std::vector<std::vector<EvidenceCoverage>>> coverage;
        std::map<std::string, std::map<std::string, std::vector<std::vector<EvidenceCoverage>>>> coverage_by_type;
        std::vector<std::string> methods;
        Json questions = Json::array(), excluded = Json::array();
        std::size_t reordered = 0;
        for (const auto& id : question_ids) {
            const auto found = entries.find(id);
            if (found == entries.end()) throw std::runtime_error("Question not in dataset: " + id);
            bool moved = false;
            const auto entry = sort_sessions_by_date(found->second->dump(), moved);
            reordered += moved;
            const auto singleton = "[" + entry + "]";
            ltm::LoadedLongMemEvalCase data;
            try {
                data = ltm::load_longmemeval_case(singleton, id,
                    {source_uri, provenance.at("revision").get<std::string>(), ltm::sha256(singleton)});
            } catch (const std::invalid_argument& e) {
                excluded.push_back({{"question_id", id}, {"question_type", found->second->at("question_type")},
                    {"reason", e.what()}});
                std::cerr << "excluded " << id << ": " << e.what() << '\n';
                continue;
            }
            const auto case_started = std::chrono::steady_clock::now();
            const auto results = evaluate_case(data, cached.get(), options["--query-prefix"], budgets);
            const auto case_ms = std::chrono::duration<double, std::milli>(std::chrono::steady_clock::now() - case_started).count();
            Json methods_json = Json::object();
            for (const auto& result : results) {
                if (std::find(methods.begin(), methods.end(), result.method) == methods.end()) methods.push_back(result.method);
                turn5[result.method].push_back(result.turn_at_5);
                turn10[result.method].push_back(result.turn_at_10);
                session5[result.method].push_back(result.session_at_5);
                auto& per_budget = coverage[result.method];
                per_budget.resize(budgets.size());
                auto& per_type = coverage_by_type[result.method][data.evaluator.question_type];
                per_type.resize(budgets.size());
                Json budget_json = Json::array();
                for (std::size_t b = 0; b < budgets.size(); ++b) {
                    per_budget[b].push_back(result.budgets[b]);
                    per_type[b].push_back(result.budgets[b]);
                    budget_json.push_back({{"budget_tokens", budgets[b]}, {"evidence", result.budgets[b].evidence},
                        {"included", result.budgets[b].included}});
                }
                methods_json[result.method] = {{"turn_at_5", metrics_json(result.turn_at_5)},
                    {"turn_at_10", metrics_json(result.turn_at_10)}, {"session_at_5", metrics_json(result.session_at_5)},
                    {"budgets", budget_json}};
            }
            std::size_t rounds_count = 0;
            for (const auto& session : data.candidate.sessions)
                for (const auto& message : session.messages) rounds_count += message.role == "user";
            questions.push_back({{"question_id", id}, {"question_type", data.evaluator.question_type},
                {"abstention", data.evaluator.abstention}, {"sessions_reordered", moved},
                {"singleton_sha256", data.actual_sha256},
                {"user_rounds", rounds_count}, {"elapsed_ms", case_ms}, {"methods", methods_json}});
            std::cerr << "done " << id << " (" << questions.size() << "/" << question_ids.size() << ", "
                      << static_cast<long>(case_ms) << " ms)\n";
        }

        Json summary = Json::object();
        for (const auto& method : methods) {
            Json budget_summary = Json::array();
            for (std::size_t b = 0; b < budgets.size(); ++b) {
                const auto s = summarize_coverage(coverage[method][b]);
                budget_summary.push_back({{"budget_tokens", budgets[b]}, {"questions", s.questions},
                    {"mean_fraction", optional_number(s.mean_fraction)}, {"all_included_rate", optional_number(s.all_included_rate)}});
            }
            Json by_type = Json::object();
            for (const auto& [type, per_budget] : coverage_by_type[method]) {
                Json rates = Json::array();
                for (std::size_t b = 0; b < budgets.size(); ++b) {
                    const auto s = summarize_coverage(per_budget[b]);
                    rates.push_back({{"budget_tokens", budgets[b]}, {"questions", s.questions},
                        {"all_included_rate", optional_number(s.all_included_rate)}});
                }
                by_type[type] = rates;
            }
            summary[method] = {{"turn_at_5", aggregate_json(question_ids, turn5[method])},
                {"turn_at_10", aggregate_json(question_ids, turn10[method])},
                {"session_at_5", aggregate_json(question_ids, session5[method])}, {"budgets", budget_summary},
                {"budgets_by_type", by_type}};
        }
        Json embedding = nullptr;
        if (http) embedding = {{"url", options["--embeddings-url"]}, {"model", options["--embedding-model"]},
            {"query_prefix", options["--query-prefix"]}, {"truncated_texts", http->truncated()}};
        const auto elapsed_s = std::chrono::duration<double>(std::chrono::steady_clock::now() - started).count();
        const Json output = {{"schema_version", 1}, {"tool", "ltm-retrieval-eval"},
            {"dataset", {{"source_uri", source_uri}, {"revision", provenance.at("revision")},
                {"sha256", provenance.at("sha256")}, {"bytes", provenance.at("bytes")}}},
            {"question_ids", question_ids}, {"budgets_tokens", budgets},
            {"token_estimate", "ceil(UTF-8 bytes / 4) per round (user message plus replies)"},
            {"ranking_text", "user message only (official LongMemEval user-turn corpus); query is the question"},
            {"bm25", {{"k1", 1.2}, {"b", 0.75}, {"idf", "ln(1 + (N - n + 0.5) / (n + 0.5))"}}},
            {"adaptations", {{"sessions_sorted_by_date", reordered},
                {"note", "Sessions stable-sorted by date before the P02 loader; questions it still rejects are listed in excluded"}}},
            {"rrf_k", 60}, {"embedding", embedding}, {"elapsed_s", elapsed_s}, {"excluded", excluded},
            {"summary", summary}, {"questions", questions}};
        std::ofstream(options["--out"], std::ios::binary | std::ios::trunc) << output.dump(2) << '\n';

        std::cout << "| Method | Turn recall@5 | Turn recall@10 | Turn NDCG@10 | Session recall@5 |";
        for (const auto budget : budgets) std::cout << " All evidence in " << budget << " tokens |";
        std::cout << "\n|---|---|---|---|---|";
        for (std::size_t b = 0; b < budgets.size(); ++b) std::cout << "---|";
        std::cout << '\n';
        for (const auto& method : methods) {
            const auto& s = summary[method];
            std::cout << "| " << method << " | " << percent(s["turn_at_5"]["recall_all"]) << " | "
                      << percent(s["turn_at_10"]["recall_all"]) << " | " << percent(s["turn_at_10"]["ndcg"]) << " | "
                      << percent(s["session_at_5"]["recall_all"]) << " |";
            for (const auto& b : s["budgets"]) std::cout << ' ' << percent(b["all_included_rate"]) << " |";
            std::cout << '\n';
        }
        std::cout << "\nQuestions: " << question_ids.size() << ", loaded: " << questions.size() << ", excluded: "
                  << excluded.size() << ", histories re-sorted: " << reordered << ", scored for retrieval: "
                  << summary[methods.front()]["turn_at_5"]["eligible"] << ". Elapsed " << static_cast<long>(elapsed_s) << " s.\n";
    } catch (const std::exception& e) {
        std::cerr << "ltm-retrieval-eval failed: " << e.what() << '\n';
        return 1;
    }
    return 0;
}
