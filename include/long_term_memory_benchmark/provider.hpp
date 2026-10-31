#pragma once
#include "experiment.hpp"
#include <chrono>
#include <functional>
#include <mutex>
#include <optional>
#include <string>
#include <vector>

namespace ltm {
using ProviderClock = std::chrono::steady_clock;
struct RequestControl {
    std::function<bool()> cancelled = [] { return false; };
    std::function<ProviderClock::time_point()> now = [] { return ProviderClock::now(); };
    std::optional<ProviderClock::time_point> deadline;
    bool stop_requested() const;
};
enum class ModelRole { reader, judge };
enum class ProviderStatus {
    success,
    safe_transient,
    authentication_error,
    configuration_error,
    billing_error,
    uncertain,
    cancelled
};
struct ProviderUsage {
    std::uint64_t input_tokens = 0;
    std::uint64_t output_tokens = 0;
};
struct ProviderRequest {
    std::uint32_t protocol_version = 1;
    std::string dispatch_id;
    std::string case_id;
    std::string prompt;
    ModelIdentity model;
    std::uint64_t output_token_limit = 0;
    std::uint64_t response_byte_limit = 0;
    std::uint64_t input_token_upper_bound = 0;
};
struct ProviderReply {
    ProviderStatus status = ProviderStatus::uncertain;
    std::string request_id;
    std::string text;
    std::string error;
    std::optional<ProviderUsage> usage;
};
class Provider {
  public:
    virtual ~Provider() = default;
    virtual ProviderReply generate(const ProviderRequest&, const RequestControl&) = 0;
};
struct ProviderPolicy {
    std::uint64_t max_requests = 0;
    std::uint64_t max_tokens = 0;
    std::uint64_t max_cost_nanos = 0;
    std::uint64_t input_token_price_nanos = 0;
    std::uint64_t output_token_price_nanos = 0;
    std::uint64_t request_price_nanos = 0;
    std::uint64_t max_prompt_bytes = 1024 * 1024;
    std::uint64_t max_response_bytes = 1024 * 1024;
    std::uint32_t max_attempts = 1;
};
struct BudgetState {
    std::uint64_t dispatched_requests = 0;
    std::uint64_t accounted_tokens = 0;
    std::uint64_t accounted_cost_nanos = 0;
    bool usage_overrun = false;
    bool accounting_overflow = false;
};
struct ProviderAttempt {
    std::string dispatch_id;
    std::string prompt_sha256;
    std::string response_text_sha256;
    ProviderRequest request;
    ProviderReply reply;
    std::uint64_t reserved_tokens = 0;
    std::uint64_t reserved_cost_nanos = 0;
    bool usage_reported = false;
    bool completed_after_deadline = false;
};
struct ProviderOutcome {
    CaseRecord record;
    std::string configuration_hash;
    std::string lifecycle_configuration_hash;
    std::vector<ProviderAttempt> attempts;
};
class ProviderRunner {
  public:
    ProviderRunner(ExperimentManifest manifest, ProviderPolicy policy);
    ProviderOutcome run(Provider&, const std::string& case_id, ModelRole role,
                        const std::string& prompt, std::uint64_t input_token_upper_bound,
                        std::uint64_t requested_output_tokens, const RequestControl& control = {});
    BudgetState budget() const;
    bool stopped() const;

  private:
    ExperimentManifest manifest_;
    ProviderPolicy policy_;
    BudgetState budget_;
    std::string configuration_hash_;
    std::string lifecycle_hash_;
    bool stopped_ = false;
    mutable std::mutex mutex_;
};
} // namespace ltm
