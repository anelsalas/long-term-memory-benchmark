#include "long_term_memory_benchmark/hash.hpp"
#include "long_term_memory_benchmark/provider.hpp"
#include <functional>
#include <iostream>
#include <limits>
#include <stdexcept>
#include <thread>

namespace {
void check(bool ok, const char* message) {
    if (!ok) {
        throw std::runtime_error(message);
    }
}
ltm::ExperimentManifest experiment_manifest() {
    ltm::ExperimentManifest experiment;
    experiment.source_revision = "test";
    experiment.dataset = {"fiction", "v1", ltm::sha256("data")};
    experiment.case_ids = {"q1", "q2"};
    experiment.method = "reference";
    experiment.method_revision = "v1";
    experiment.reader = {"stub", "reader", "v1"};
    experiment.judge = {"stub", "judge", "v1"};
    experiment.prompt_sha256 = {{"answer", ltm::sha256("prompt")}};
    experiment.context_token_limit = 100;
    experiment.output_token_limit = 20;
    return experiment;
}
ltm::ProviderPolicy provider_policy() {
    ltm::ProviderPolicy limits;
    limits.max_requests = 5;
    limits.max_tokens = 500;
    limits.max_cost_nanos = 5000;
    limits.input_token_price_nanos = 2;
    limits.output_token_price_nanos = 3;
    limits.request_price_nanos = 1;
    limits.max_prompt_bytes = 1000;
    limits.max_response_bytes = 1000;
    limits.max_attempts = 3;
    return limits;
}
struct StubProvider : ltm::Provider {
    std::vector<ltm::ProviderReply> replies;
    std::size_t calls = 0;
    bool throw_on_call = false;
    std::function<void()> during = [] {};
    std::vector<ltm::ProviderRequest> requests;
    ltm::ProviderReply generate(const ltm::ProviderRequest& request,
                                const ltm::RequestControl&) override {
        ++calls;
        requests.push_back(request);
        during();
        if (throw_on_call) {
            throw std::runtime_error("unavailable");
        }
        return replies.at(calls - 1);
    }
};
ltm::ProviderReply successful_reply() {
    return {ltm::ProviderStatus::success, "remote-id", "91", "", ltm::ProviderUsage{4, 2}};
}
void success_accounting() {
    StubProvider stub;
    stub.replies = {successful_reply()};
    ltm::ProviderRunner runner(experiment_manifest(), provider_policy());
    const auto outcome = runner.run(stub, "q1", ltm::ModelRole::reader, "question", 10, 5);
    check(outcome.record.status == ltm::CaseStatus::completed && outcome.record.answer == "91" &&
              !outcome.record.correct,
          "Completed ungraded answer");
    check(outcome.record.request_id == "remote-id" && outcome.attempts.size() == 1,
          "Provider identity saved");
    check(runner.budget().dispatched_requests == 1 && runner.budget().accounted_tokens == 6 &&
              runner.budget().accounted_cost_nanos == 15,
          "Reported usage settles reservation");
    check(stub.requests[0].output_token_limit == 5 &&
              outcome.configuration_hash == ltm::configuration_hash(experiment_manifest()),
          "Manifest and output limit preserved");
}
void budget_caps() {
    for (int cap = 0; cap < 3; ++cap) {
        auto limits = provider_policy();
        if (cap == 0) {
            limits.max_requests = 0;
        }
        if (cap == 1) {
            limits.max_tokens = 14;
        }
        if (cap == 2) {
            limits.max_cost_nanos = 35;
        }
        StubProvider stub;
        ltm::ProviderRunner runner(experiment_manifest(), limits);
        auto outcome = runner.run(stub, "q1", ltm::ModelRole::reader, "question", 10, 5);
        check(stub.calls == 0 && outcome.record.status == ltm::CaseStatus::failed,
              "Reserve worst-case budget before dispatch");
    }
    auto limits = provider_policy();
    limits.max_requests = 1;
    StubProvider stub;
    stub.replies = {successful_reply()};
    ltm::ProviderRunner runner(experiment_manifest(), limits);
    runner.run(stub, "q1", ltm::ModelRole::reader, "q", 10, 5);
    auto outcome = runner.run(stub, "q2", ltm::ModelRole::reader, "q", 10, 5);
    check(stub.calls == 1 && outcome.record.status == ltm::CaseStatus::failed,
          "Exhausted request cap stops dispatch");
}
void retry_safety() {
    StubProvider stub;
    stub.replies = {{ltm::ProviderStatus::safe_transient, "", "", "busy", {}}, successful_reply()};
    ltm::ProviderRunner runner(experiment_manifest(), provider_policy());
    auto outcome = runner.run(stub, "q1", ltm::ModelRole::reader, "q", 10, 5);
    check(stub.calls == 2 && outcome.record.status == ltm::CaseStatus::completed &&
              outcome.attempts.size() == 2,
          "Safe transient retries");
    check(stub.requests[0].dispatch_id != stub.requests[1].dispatch_id, "Attempt IDs are distinct");
    auto limits = provider_policy();
    limits.max_attempts = 2;
    StubProvider limited;
    limited.replies = {stub.replies[0], stub.replies[0]};
    ltm::ProviderRunner bounded(experiment_manifest(), limits);
    outcome = bounded.run(limited, "q1", ltm::ModelRole::reader, "q", 10, 5);
    check(limited.calls == 2 && outcome.record.status == ltm::CaseStatus::failed,
          "Attempt count bounded");
}
void uncertain_submission() {
    for (bool throws : {false, true}) {
        StubProvider stub;
        stub.throw_on_call = throws;
        stub.replies = {{ltm::ProviderStatus::uncertain, "request-maybe", "", "timeout", {}}};
        ltm::ProviderRunner runner(experiment_manifest(), provider_policy());
        auto outcome = runner.run(stub, "q1", ltm::ModelRole::reader, "q", 10, 5);
        check(stub.calls == 1 && outcome.record.status == ltm::CaseStatus::uncertain,
              "Ambiguous submission awaits reconciliation");
        check(runner.budget().accounted_tokens == 15 && runner.budget().accounted_cost_nanos == 36,
              "Unknown usage retains reservation");
        auto again = runner.run(stub, "q1", ltm::ModelRole::reader, "q", 10, 5);
        check(stub.calls == 1 && again.record.status == ltm::CaseStatus::failed,
              "Uncertain runner blocks duplicate submissions");
    }
}
void fatal_failure() {
    for (auto status :
         {ltm::ProviderStatus::authentication_error, ltm::ProviderStatus::configuration_error,
          ltm::ProviderStatus::billing_error}) {
        StubProvider stub;
        stub.replies = {{status, "rejected", "", "fatal", ltm::ProviderUsage{2, 0}}};
        ltm::ProviderRunner runner(experiment_manifest(), provider_policy());
        auto outcome = runner.run(stub, "q1", ltm::ModelRole::reader, "q", 10, 5);
        runner.run(stub, "q2", ltm::ModelRole::reader, "q", 10, 5);
        check(stub.calls == 1 && outcome.record.status == ltm::CaseStatus::failed &&
                  runner.stopped(),
              "Fatal category stops run");
        check(runner.budget().accounted_tokens == 2 && runner.budget().accounted_cost_nanos == 5,
              "Failure usage recorded");
    }
}
void cancellation_deadline() {
    for (bool cancellation : {false, true}) {
        ltm::RequestControl control;
        if (cancellation) {
            control.cancelled = [] { return true; };
        } else {
            control.now = [] { return ltm::ProviderClock::time_point{}; };
            control.deadline = ltm::ProviderClock::time_point{};
        }
        StubProvider stub;
        ltm::ProviderRunner runner(experiment_manifest(), provider_policy());
        auto outcome = runner.run(stub, "q1", ltm::ModelRole::reader, "q", 10, 5, control);
        check(stub.calls == 0 && outcome.record.status == ltm::CaseStatus::failed,
              "Cancellation/deadline checked before dispatch");
    }
    auto now = ltm::ProviderClock::time_point{};
    ltm::RequestControl control;
    control.now = [&] { return now; };
    control.deadline = now + std::chrono::milliseconds(1);
    StubProvider stub;
    stub.replies = {successful_reply()};
    stub.during = [&] { now += std::chrono::milliseconds(2); };
    ltm::ProviderRunner runner(experiment_manifest(), provider_policy());
    auto outcome = runner.run(stub, "q1", ltm::ModelRole::reader, "q", 10, 5, control);
    check(outcome.record.status == ltm::CaseStatus::completed &&
              outcome.attempts[0].completed_after_deadline,
          "Late synchronous result identified");
}
void malformed_reply() {
    for (const auto& reply : std::vector<ltm::ProviderReply>{
             {ltm::ProviderStatus::success, "id", "", "", {}},
             {ltm::ProviderStatus::success, "id", std::string(1001, 'x'), "", {}},
             {ltm::ProviderStatus::success, "id", std::string(1, static_cast<char>(0xff)), "", {}},
             {static_cast<ltm::ProviderStatus>(99), "id", "answer", "", {}}}) {
        StubProvider stub;
        stub.replies = {reply};
        ltm::ProviderRunner runner(experiment_manifest(), provider_policy());
        const auto outcome = runner.run(stub, "q1", ltm::ModelRole::reader, "q", 10, 5);
        check(stub.calls == 1 && outcome.record.status == ltm::CaseStatus::uncertain,
              "Malformed reply retains dispatch and requires reconciliation");
    }
}
void limits_overflow() {
    StubProvider stub;
    ltm::ProviderRunner runner(experiment_manifest(), provider_policy());
    const auto outcome = runner.run(stub, "q1", ltm::ModelRole::reader, "q", 10, 21);
    check(stub.calls == 0 && outcome.record.status == ltm::CaseStatus::failed,
          "Requested output bounded by manifest");
    auto limits = provider_policy();
    limits.input_token_price_nanos = std::numeric_limits<std::uint64_t>::max();
    ltm::ProviderRunner overflow(experiment_manifest(), limits);
    check(overflow.run(stub, "q1", ltm::ModelRole::reader, "q", 10, 5).record.status ==
                  ltm::CaseStatus::failed &&
              stub.calls == 0,
          "Overflow rejected before dispatch");
    StubProvider over_budget_provider;
    over_budget_provider.replies = {
        {ltm::ProviderStatus::success, "id", "91", "", ltm::ProviderUsage{101, 2}}};
    ltm::ProviderRunner over_budget_runner(experiment_manifest(), provider_policy());
    auto actual =
        over_budget_runner.run(over_budget_provider, "q1", ltm::ModelRole::reader, "q", 10, 5);
    check(actual.record.status == ltm::CaseStatus::uncertain &&
              over_budget_runner.budget().usage_overrun && over_budget_runner.stopped(),
          "Reported usage exceeding reservation stops run");
}
void missing_usage_and_identity() {
    StubProvider stub;
    auto reply = successful_reply();
    reply.usage.reset();
    stub.replies = {reply};
    ltm::ProviderRunner runner(experiment_manifest(), provider_policy());
    auto outcome = runner.run(stub, "q1", ltm::ModelRole::judge, "q", 10, 5);
    check(outcome.record.status == ltm::CaseStatus::completed &&
              !outcome.attempts[0].usage_reported,
          "Unreported success usage explicitly unknown");
    check(runner.budget().accounted_tokens == 15 && runner.budget().accounted_cost_nanos == 36,
          "Successful unknown usage retains maximum reservation");
    check(outcome.attempts[0].request.input_token_upper_bound == 10 &&
              outcome.attempts[0].request.model.id == "judge",
          "Actual request cap and role recorded");
    auto changed = provider_policy();
    changed.max_response_bytes = 900;
    ltm::ProviderRunner other(experiment_manifest(), changed);
    StubProvider second;
    second.replies = {successful_reply()};
    const auto different = other.run(second, "q1", ltm::ModelRole::judge, "q", 10, 5);
    check(outcome.lifecycle_configuration_hash != different.lifecycle_configuration_hash,
          "Policy changes complete lifecycle identity");
    StubProvider missing_id_provider;
    auto transient =
        ltm::ProviderReply{ltm::ProviderStatus::safe_transient, "first-id", "", "retry", {}};
    auto accepted = successful_reply();
    accepted.request_id.clear();
    missing_id_provider.replies = {transient, accepted};
    ltm::ProviderRunner identity_runner(experiment_manifest(), provider_policy());
    const auto last =
        identity_runner.run(missing_id_provider, "q1", ltm::ModelRole::reader, "q", 10, 5);
    check(last.record.request_id.empty() && last.attempts[0].reply.request_id == "first-id",
          "Prior attempt ID does not become final response identity");
}
void after_dispatch_cancellation_and_overflow() {
    StubProvider cancelled;
    cancelled.replies = {{ltm::ProviderStatus::cancelled, "maybe", "", "cancelled after send", {}}};
    ltm::ProviderRunner runner(experiment_manifest(), provider_policy());
    check(runner.run(cancelled, "q1", ltm::ModelRole::reader, "q", 10, 5).record.status ==
              ltm::CaseStatus::uncertain,
          "After-send cancellation preserves uncertainty");
    StubProvider overflow;
    overflow.replies = {{ltm::ProviderStatus::success, "id", "91", "",
                         ltm::ProviderUsage{std::numeric_limits<std::uint64_t>::max(), 1}}};
    ltm::ProviderRunner accounting(experiment_manifest(), provider_policy());
    auto outcome = accounting.run(overflow, "q1", ltm::ModelRole::reader, "q", 10, 5);
    check(outcome.record.status == ltm::CaseStatus::uncertain &&
              accounting.budget().accounting_overflow,
          "Unrepresentable reported usage is explicit and stops dispatch");
}
void concurrent_budget() {
    auto limits = provider_policy();
    limits.max_requests = 1;
    StubProvider stub;
    stub.replies = {successful_reply()};
    ltm::ProviderRunner runner(experiment_manifest(), limits);
    ltm::ProviderOutcome first, second;
    std::thread first_worker(
        [&] { first = runner.run(stub, "q1", ltm::ModelRole::reader, "q", 10, 5); });
    std::thread second_worker(
        [&] { second = runner.run(stub, "q2", ltm::ModelRole::reader, "q", 10, 5); });
    first_worker.join();
    second_worker.join();
    check(stub.calls == 1 && runner.budget().dispatched_requests == 1,
          "One shared runner serializes callers before reservation");
    check((first.record.status == ltm::CaseStatus::completed) +
                  (second.record.status == ltm::CaseStatus::completed) ==
              1,
          "Only one concurrent request completes under one-request cap");
}
void invalid_request_and_control() {
    StubProvider stub;
    ltm::ProviderRunner runner(experiment_manifest(), provider_policy());
    check(runner.run(stub, "foreign", ltm::ModelRole::reader, "q", 10, 5).record.status ==
              ltm::CaseStatus::failed,
          "Unknown case rejected");
    check(runner.run(stub, "q1", ltm::ModelRole::reader, std::string(1001, 'q'), 10, 5)
                  .record.status == ltm::CaseStatus::failed,
          "Prompt byte cap enforced");
    check(runner.run(stub, "q1", ltm::ModelRole::reader, "q", 101, 5).record.status ==
              ltm::CaseStatus::failed,
          "Context bound enforced");
    ltm::RequestControl control;
    control.cancelled = [] {
        throw std::runtime_error("control");
        return false;
    };
    check(runner.run(stub, "q1", ltm::ModelRole::reader, "q", 10, 5, control).record.status ==
                  ltm::CaseStatus::failed &&
              stub.calls == 0,
          "Invalid control fails before dispatch");
    auto limits = provider_policy();
    limits.max_attempts = 0;
    bool threw = false;
    try {
        ltm::ProviderRunner invalid(experiment_manifest(), limits);
    } catch (const std::invalid_argument&) {
        threw = true;
    }
    check(threw, "Zero attempt policy rejected");
}
} // namespace
int main() {
    const std::vector<std::pair<const char*, std::function<void()>>> tests = {
        {"success_accounting", success_accounting},
        {"budget_caps", budget_caps},
        {"retry_safety", retry_safety},
        {"uncertain_submission", uncertain_submission},
        {"fatal_failure", fatal_failure},
        {"cancellation_deadline", cancellation_deadline},
        {"malformed_reply", malformed_reply},
        {"limits_overflow", limits_overflow},
        {"missing_usage_and_identity", missing_usage_and_identity},
        {"after_dispatch_cancellation_and_overflow", after_dispatch_cancellation_and_overflow},
        {"concurrent_budget", concurrent_budget},
        {"invalid_request_and_control", invalid_request_and_control}};
    int failed = 0;
    for (const auto& test : tests) {
        try {
            test.second();
            std::cout << "PASS " << test.first << '\n';
        } catch (const std::exception& e) {
            ++failed;
            std::cerr << "FAIL " << test.first << ": " << e.what() << '\n';
        }
    }
    return failed ? 1 : 0;
}
