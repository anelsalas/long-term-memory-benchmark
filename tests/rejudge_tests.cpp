#include "long_term_memory_benchmark/rejudge.hpp"
#include <nlohmann/json.hpp>
#include <functional>
#include <iostream>
#include <stdexcept>
#include <string>
#include <utility>
#include <vector>

namespace {
using namespace ltm::rejudge;
using Json = nlohmann::json;

void require(bool condition, const std::string& message) {
    if (!condition) throw std::runtime_error(message);
}
template<class F> bool rejected(F action) {
    try { action(); } catch (const std::invalid_argument&) { return true; }
    return false;
}

class FakeJudge : public ltm::Provider {
  public:
    explicit FakeJudge(ltm::ProviderReply reply) : reply_(std::move(reply)) {}
    ltm::ProviderReply generate(const ltm::ProviderRequest& request, const ltm::RequestControl&) override {
        requests.push_back(request);
        return reply_;
    }
    std::vector<ltm::ProviderRequest> requests;
  private:
    ltm::ProviderReply reply_;
};
ltm::ProviderReply success(const std::string& text) {
    return {ltm::ProviderStatus::success, "r-1", text, "", ltm::ProviderUsage{120, 3}};
}
const ltm::ModelIdentity judge_model{"openai-compatible", "gpt-6.1-sol/high", "gpt-6.1-sol/high"};

void items_are_read() {
    const auto items = parse_items(R"({"items":[{"key":"q1|dense_budget","prompt":"Is it right? Answer yes or no only."},
        {"key":"q1|known_evidence","prompt":"Second"}]})");
    require(items.size() == 2 && items[0].key == "q1|dense_budget" && items[1].prompt == "Second", "Keys and prompts in order");
    require(rejected([] { parse_items(R"({"items":[{"key":"a","prompt":"x"},{"key":"a","prompt":"y"}]})"); }), "Duplicate keys");
    require(rejected([] { parse_items(R"({"items":[{"key":"a"}]})"); }), "Missing prompt");
    require(rejected([] { parse_items("[]"); }), "Not an item list");
}
void the_frozen_prompt_is_sent_unchanged() {
    FakeJudge judge(success("Yes"));
    const Item item{"q1|dense_budget", "Exact frozen prompt\nAnswer yes or no only."};
    const auto verdict = judge_item(judge, judge_model, item, 16000);
    require(judge.requests.size() == 1 && judge.requests[0].prompt == item.prompt, "Prompt bytes are unchanged");
    require(judge.requests[0].model.version == "gpt-6.1-sol/high" && judge.requests[0].output_token_limit == 16000,
        "Judge model and output cap");
    require(judge.requests[0].case_id == item.key, "The key identifies the request");
    require(verdict.key == item.key && verdict.verdict == "yes" && verdict.reply == "Yes", "Strict verdict");
    require(verdict.usage && verdict.usage->input_tokens == 120, "Usage is kept");
}
void verdicts_follow_the_strict_parser() {
    FakeJudge no(success(" no\n"));
    require(judge_item(no, judge_model, {"k", "p"}, 10).verdict == "no", "No");
    FakeJudge rambling(success("Yes, the answer matches."));
    require(judge_item(rambling, judge_model, {"k", "p"}, 10).verdict == "invalid", "Anything but yes or no is invalid");
    FakeJudge broken({ltm::ProviderStatus::uncertain, "", "", "HTTP 502; provider response body withheld", std::nullopt});
    const auto failed = judge_item(broken, judge_model, {"k", "p"}, 10);
    require(failed.verdict == "failed" && failed.error == "HTTP 502; provider response body withheld", "Failures are kept");
}
void results_round_trip_and_resume() {
    const std::vector<Verdict> verdicts{{"a", "yes", "Yes", "", ltm::ProviderUsage{1, 1}, 5.0},
        {"b", "failed", "", "timeout", std::nullopt, 9.0}};
    const auto text = results_json("gpt-6.1-sol/high", verdicts);
    const auto json = Json::parse(text);
    require(json["judge"] == "gpt-6.1-sol/high" && json["verdicts"].size() == 2, "Judge and verdicts");
    require(json["counts"]["yes"] == 1 && json["counts"]["failed"] == 1, "Counts by verdict");
    const auto done = completed_verdicts(text);
    require(done.size() == 1 && done[0].key == "a", "Resume keeps finished verdicts and retries failures");
}
}

int main(int argc, char** argv) {
    const std::vector<std::pair<std::string, std::function<void()>>> cases = {
        {"items_are_read", items_are_read}, {"the_frozen_prompt_is_sent_unchanged", the_frozen_prompt_is_sent_unchanged},
        {"verdicts_follow_the_strict_parser", verdicts_follow_the_strict_parser},
        {"results_round_trip_and_resume", results_round_trip_and_resume}};
    if (argc != 2) return 2;
    for (const auto& item : cases) {
        if (item.first == argv[1]) {
            try { item.second(); std::cout << "PASS " << item.first << '\n'; return 0; }
            catch (const std::exception& e) { std::cerr << "FAIL " << item.first << ": " << e.what() << '\n'; return 1; }
        }
    }
    return 2;
}
