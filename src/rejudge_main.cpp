// ltm-rejudge --items FILE --endpoint URL --judge MODEL --out FILE [--workers N]
//             [--output-tokens N] [--timeout-ms N]
// Sends saved official judge prompts to a loopback judge (local execution mode)
// and writes strict verdicts. Rerunning with the same --out retries only failures.
#include "long_term_memory_benchmark/http_provider.hpp"
#include "long_term_memory_benchmark/rejudge.hpp"
#include <atomic>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <map>
#include <mutex>
#include <set>
#include <sstream>
#include <string>
#include <thread>
#include <vector>

namespace {
std::string read_file(const std::filesystem::path& path) {
    std::ifstream in(path, std::ios::binary);
    if (!in) throw std::runtime_error("Cannot read " + path.string());
    std::ostringstream text;
    text << in.rdbuf();
    return text.str();
}
void write_file(const std::filesystem::path& path, const std::string& text) {
    const auto temporary = path.string() + ".tmp";
    std::ofstream(temporary, std::ios::binary | std::ios::trunc) << text;
    std::filesystem::rename(temporary, path);
}
}

int main(int argc, char** argv) {
    try {
        std::map<std::string, std::string> options{{"--workers", "1"}, {"--output-tokens", "16000"},
            {"--timeout-ms", "600000"}};
        for (int i = 1; i + 1 < argc; i += 2) options[argv[i]] = argv[i + 1];
        for (const auto* required : {"--items", "--endpoint", "--judge", "--out"})
            if (!options.count(required) || argc % 2 == 0)
                throw std::invalid_argument("Usage: ltm-rejudge --items FILE --endpoint URL --judge MODEL --out FILE "
                                            "[--workers N] [--output-tokens N] [--timeout-ms N]");
        const auto items = ltm::rejudge::parse_items(read_file(options["--items"]));
        const std::filesystem::path out = options["--out"];
        std::vector<ltm::rejudge::Verdict> verdicts;
        if (std::filesystem::exists(out)) verdicts = ltm::rejudge::completed_verdicts(read_file(out));
        std::set<std::string> done;
        for (const auto& verdict : verdicts) done.insert(verdict.key);
        std::vector<const ltm::rejudge::Item*> pending;
        for (const auto& item : items)
            if (!done.count(item.key)) pending.push_back(&item);

        ltm::ChatCompletionsOptions http;
        http.endpoint_url = options["--endpoint"];
        http.output_token_parameter = "max_tokens";
        http.temperature = 0;  // the official judge temperature; recorded even if a CLI judge cannot apply it
        http.timeout_ms = static_cast<std::uint32_t>(std::stoul(options["--timeout-ms"]));
        http.local_mode = true;
        ltm::validate_chat_endpoint(http.endpoint_url, true);
        const ltm::ModelIdentity model{"openai-compatible", options["--judge"], options["--judge"]};
        const auto output_tokens = std::stoull(options["--output-tokens"]);

        std::mutex mutex;
        std::atomic<std::size_t> next{0};
        std::vector<std::thread> workers;
        const auto worker_count = std::max<std::size_t>(1, std::stoul(options["--workers"]));
        for (std::size_t w = 0; w < worker_count; ++w) {
            workers.emplace_back([&] {
                ltm::WinHttpTransport transport;
                ltm::ChatCompletionsProvider judge(http, "", transport);
                for (std::size_t i = next++; i < pending.size(); i = next++) {
                    auto verdict = ltm::rejudge::judge_item(judge, model, *pending[i], output_tokens);
                    std::lock_guard<std::mutex> lock(mutex);
                    std::cout << verdict.key << ' ' << verdict.verdict << std::endl;
                    verdicts.push_back(std::move(verdict));
                    write_file(out, ltm::rejudge::results_json(model.version, verdicts));
                }
            });
        }
        for (auto& worker : workers) worker.join();
        write_file(out, ltm::rejudge::results_json(model.version, verdicts));
        std::size_t failed = 0;
        for (const auto& verdict : verdicts) failed += verdict.verdict == "failed";
        std::cout << "judged " << verdicts.size() << " of " << items.size() << ", failed " << failed << std::endl;
        return failed || verdicts.size() != items.size() ? 3 : 0;
    } catch (const std::exception& e) {
        std::cerr << "ltm-rejudge: " << e.what() << '\n';
        return 2;
    }
}
