#include "long_term_memory_benchmark/provider.hpp"
#include "long_term_memory_benchmark/hash.hpp"
#include <algorithm>
#include <limits>
#include <stdexcept>
#include <utility>

namespace ltm {
namespace {
bool checked_add(std::uint64_t left, std::uint64_t right, std::uint64_t& result) {
    if (right > std::numeric_limits<std::uint64_t>::max() - left) {
        return false;
    }
    result = left + right;
    return true;
}

bool checked_multiply(std::uint64_t left, std::uint64_t right, std::uint64_t& result) {
    if (left != 0 && right > std::numeric_limits<std::uint64_t>::max() / left) {
        return false;
    }
    result = left * right;
    return true;
}

bool calculate_charge(const ProviderPolicy& policy, const ProviderUsage& usage,
                      std::uint64_t& tokens, std::uint64_t& cost) {
    std::uint64_t input_cost = 0;
    std::uint64_t output_cost = 0;
    std::uint64_t token_cost = 0;
    return checked_add(usage.input_tokens, usage.output_tokens, tokens) &&
           checked_multiply(usage.input_tokens, policy.input_token_price_nanos, input_cost) &&
           checked_multiply(usage.output_tokens, policy.output_token_price_nanos, output_cost) &&
           checked_add(input_cost, output_cost, token_cost) &&
           checked_add(token_cost, policy.request_price_nanos, cost);
}

bool is_valid_utf8(const std::string& text) {
    std::size_t byte_index = 0;
    while (byte_index < text.size()) {
        const auto leading_byte = static_cast<unsigned char>(text[byte_index++]);
        if (leading_byte < 0x80) {
            continue;
        }
        std::uint32_t code_point = 0;
        std::uint32_t minimum_code_point = 0;
        unsigned continuation_count = 0;
        if (leading_byte >= 0xc2 && leading_byte <= 0xdf) {
            code_point = leading_byte & 31;
            continuation_count = 1;
            minimum_code_point = 0x80;
        } else if (leading_byte >= 0xe0 && leading_byte <= 0xef) {
            code_point = leading_byte & 15;
            continuation_count = 2;
            minimum_code_point = 0x800;
        } else if (leading_byte >= 0xf0 && leading_byte <= 0xf4) {
            code_point = leading_byte & 7;
            continuation_count = 3;
            minimum_code_point = 0x10000;
        } else {
            return false;
        }
        if (text.size() - byte_index < continuation_count) {
            return false;
        }
        while (continuation_count > 0) {
            const auto continuation_byte = static_cast<unsigned char>(text[byte_index++]);
            if ((continuation_byte & 0xc0) != 0x80) {
                return false;
            }
            code_point = (code_point << 6) | (continuation_byte & 63);
            --continuation_count;
        }
        if (code_point < minimum_code_point || code_point > 0x10ffff ||
            (code_point >= 0xd800 && code_point <= 0xdfff)) {
            return false;
        }
    }
    return true;
}

bool is_known_provider_status(ProviderStatus status) {
    switch (status) {
    case ProviderStatus::success:
    case ProviderStatus::safe_transient:
    case ProviderStatus::authentication_error:
    case ProviderStatus::configuration_error:
    case ProviderStatus::billing_error:
    case ProviderStatus::uncertain:
    case ProviderStatus::cancelled:
        return true;
    }
    return false;
}
bool is_fatal_provider_failure(ProviderStatus status) {
    return status == ProviderStatus::authentication_error ||
           status == ProviderStatus::configuration_error || status == ProviderStatus::billing_error;
}
std::string calculate_lifecycle_hash(const std::string& manifest_hash,
                                     const ProviderPolicy& policy) {
    std::string text = "provider-lifecycle-v1|" + manifest_hash;
    for (const auto value :
         {policy.max_requests, policy.max_tokens, policy.max_cost_nanos,
          policy.input_token_price_nanos, policy.output_token_price_nanos,
          policy.request_price_nanos, policy.max_prompt_bytes, policy.max_response_bytes,
          static_cast<std::uint64_t>(policy.max_attempts)}) {
        text += '|' + std::to_string(value);
    }
    return sha256(text);
}
bool reserve_budget(BudgetState& budget, const ProviderPolicy& policy,
                    std::uint64_t reserved_tokens, std::uint64_t reserved_cost) {
    std::uint64_t next_tokens = 0;
    std::uint64_t next_cost = 0;
    if (budget.dispatched_requests >= policy.max_requests ||
        !checked_add(budget.accounted_tokens, reserved_tokens, next_tokens) ||
        next_tokens > policy.max_tokens ||
        !checked_add(budget.accounted_cost_nanos, reserved_cost, next_cost) ||
        next_cost > policy.max_cost_nanos) {
        return false;
    }
    ++budget.dispatched_requests;
    budget.accounted_tokens = next_tokens;
    budget.accounted_cost_nanos = next_cost;
    return true;
}

void settle_attempt_usage(BudgetState& budget, const ProviderPolicy& policy,
                          ProviderAttempt& attempt, std::uint64_t input_bound,
                          std::uint64_t output_limit) {
    auto& reply = attempt.reply;
    const auto reserved_tokens = attempt.reserved_tokens;
    const auto reserved_cost = attempt.reserved_cost_nanos;
    std::uint64_t next_tokens = 0;
    std::uint64_t next_cost = 0;
    std::uint64_t settled_tokens = reserved_tokens;
    std::uint64_t settled_cost = reserved_cost;
    bool overrun = false;
    if (reply.usage) {
        attempt.usage_reported = true;
        overrun =
            reply.usage->input_tokens > input_bound || reply.usage->output_tokens > output_limit;
        if (!calculate_charge(policy, *reply.usage, settled_tokens, settled_cost)) {
            budget.accounting_overflow = true;
            settled_tokens = reserved_tokens;
            settled_cost = reserved_cost;
            overrun = true;
        }
    } else if (reply.status == ProviderStatus::safe_transient) {
        settled_tokens = 0;
        settled_cost = 0;
    }
    const auto previous_tokens = budget.accounted_tokens - reserved_tokens;
    const auto previous_cost = budget.accounted_cost_nanos - reserved_cost;
    if (!checked_add(previous_tokens, settled_tokens, next_tokens) ||
        !checked_add(previous_cost, settled_cost, next_cost)) {
        budget.accounting_overflow = true;
        overrun = true;
    } else {
        budget.accounted_tokens = next_tokens;
        budget.accounted_cost_nanos = next_cost;
    }
    if (overrun || budget.accounted_tokens > policy.max_tokens ||
        budget.accounted_cost_nanos > policy.max_cost_nanos) {
        budget.usage_overrun = true;
        reply.status = ProviderStatus::uncertain;
        reply.error = "Reported usage exceeds reservation or accounting capacity; reconcile";
    }
}

void normalize_provider_reply(ProviderReply& reply, std::uint64_t response_byte_limit) {
    const auto valid_field = [response_byte_limit](const std::string& value) {
        return value.size() <= response_byte_limit && is_valid_utf8(value);
    };
    const bool fields_valid =
        valid_field(reply.text) && valid_field(reply.request_id) && valid_field(reply.error);
    const bool success_valid =
        reply.status != ProviderStatus::success || (!reply.text.empty() && reply.error.empty());
    const bool transient_valid =
        reply.status != ProviderStatus::safe_transient || reply.text.empty();
    if (is_known_provider_status(reply.status) && fields_valid && success_valid &&
        transient_valid) {
        return;
    }
    reply.status = ProviderStatus::uncertain;
    reply.error = "Malformed provider result; reconcile submission";
    reply.text.clear();
    if (!valid_field(reply.request_id)) {
        reply.request_id.clear();
    }
}

} // namespace
bool RequestControl::stop_requested() const {
    return (cancelled && cancelled()) || (deadline && now() >= *deadline);
}
ProviderRunner::ProviderRunner(ExperimentManifest manifest, ProviderPolicy policy)
    : manifest_(std::move(manifest)), policy_(policy),
      configuration_hash_(ltm::configuration_hash(manifest_)),
      lifecycle_hash_(calculate_lifecycle_hash(configuration_hash_, policy_)) {
    if (!policy_.max_attempts || !policy_.max_prompt_bytes || !policy_.max_response_bytes) {
        throw std::invalid_argument("Provider attempts and byte limits must be positive");
    }
}
BudgetState ProviderRunner::budget() const {
    std::lock_guard<std::mutex> lock(mutex_);
    return budget_;
}
bool ProviderRunner::stopped() const {
    std::lock_guard<std::mutex> lock(mutex_);
    return stopped_;
}
ProviderOutcome ProviderRunner::run(Provider& provider, const std::string& case_id, ModelRole role,
                                    const std::string& prompt, std::uint64_t input_bound,
                                    std::uint64_t output_limit, const RequestControl& control) {
    std::lock_guard<std::mutex> lock(mutex_);
    ProviderOutcome outcome;
    outcome.record.case_id = case_id;
    outcome.configuration_hash = configuration_hash_;
    outcome.lifecycle_configuration_hash = lifecycle_hash_;
    const auto fail = [&](const std::string& error) {
        outcome.record.status = CaseStatus::failed;
        outcome.record.error = error;
        return outcome;
    };
    if (stopped_) {
        return fail("Provider run stopped; reconcile or resolve failure before restarting");
    }
    if (std::find(manifest_.case_ids.begin(), manifest_.case_ids.end(), case_id) ==
        manifest_.case_ids.end()) {
        return fail("Unknown case ID");
    }
    if (role != ModelRole::reader && role != ModelRole::judge) {
        return fail("Unknown model role");
    }
    if (!control.now) {
        return fail("Clock callback required");
    }
    if (prompt.empty() || prompt.size() > policy_.max_prompt_bytes || !is_valid_utf8(prompt)) {
        return fail("Prompt invalid or exceeds byte cap");
    }
    if (!input_bound || input_bound > manifest_.context_token_limit || !output_limit ||
        output_limit > manifest_.output_token_limit) {
        return fail("Input/output token bound exceeds manifest limits");
    }
    std::uint64_t reserved_tokens = 0;
    std::uint64_t reserved_cost = 0;
    if (!calculate_charge(policy_, {input_bound, output_limit}, reserved_tokens, reserved_cost)) {
        return fail("Reservation integer overflow");
    }
    for (std::uint32_t attempt_index = 0; attempt_index < policy_.max_attempts; ++attempt_index) {
        try {
            if (control.stop_requested()) {
                return fail("Request cancelled or deadline reached before dispatch");
            }
        } catch (...) {
            return fail("Cancellation/clock callback failed before dispatch");
        }
        if (!reserve_budget(budget_, policy_, reserved_tokens, reserved_cost)) {
            return fail("Request/token/cost budget exhausted before dispatch");
        }
        ProviderRequest request;
        request.dispatch_id = lifecycle_hash_ + ":" + sha256(case_id) + ":" + sha256(prompt) + ":" +
                              (role == ModelRole::reader ? "reader:" : "judge:") +
                              std::to_string(input_bound) + ":" + std::to_string(output_limit) +
                              ":" + std::to_string(budget_.dispatched_requests);
        request.case_id = case_id;
        request.prompt = prompt;
        request.model = role == ModelRole::reader ? manifest_.reader : manifest_.judge;
        request.output_token_limit = output_limit;
        request.response_byte_limit = policy_.max_response_bytes;
        request.input_token_upper_bound = input_bound;
        ProviderAttempt attempt;
        attempt.dispatch_id = request.dispatch_id;
        attempt.prompt_sha256 = sha256(prompt);
        attempt.request = request;
        attempt.reserved_tokens = reserved_tokens;
        attempt.reserved_cost_nanos = reserved_cost;
        outcome.record.status = CaseStatus::running;
        try {
            attempt.reply = provider.generate(request, control);
        } catch (...) {
            attempt.reply = {ProviderStatus::uncertain,
                             "",
                             "",
                             "Provider threw after dispatch; reconcile submission",
                             {}};
        }
        try {
            attempt.completed_after_deadline =
                control.deadline && control.now() >= *control.deadline;
        } catch (...) {
            attempt.reply.status = ProviderStatus::uncertain;
            attempt.reply.error = "Clock callback failed after dispatch; reconcile";
        }
        auto& reply = attempt.reply;
        attempt.response_text_sha256 = sha256(reply.text);
        normalize_provider_reply(reply, policy_.max_response_bytes);
        settle_attempt_usage(budget_, policy_, attempt, input_bound, output_limit);
        if (budget_.usage_overrun) {
            stopped_ = true;
        }
        outcome.record.request_id = reply.request_id;
        outcome.attempts.push_back(std::move(attempt));
        const auto& saved = outcome.attempts.back().reply;
        if (saved.status == ProviderStatus::success) {
            outcome.record.status = CaseStatus::completed;
            outcome.record.answer = saved.text;
            outcome.record.error.clear();
            return outcome;
        }
        if (saved.status == ProviderStatus::uncertain ||
            saved.status == ProviderStatus::cancelled) {
            stopped_ = true;
            outcome.record.status = CaseStatus::uncertain;
            outcome.record.error =
                saved.error.empty() ? "Submission requires reconciliation" : saved.error;
            return outcome;
        }
        if (is_fatal_provider_failure(saved.status)) {
            stopped_ = true;
            return fail(saved.error.empty() ? "Fatal provider failure" : saved.error);
        }
        if (attempt_index + 1 == policy_.max_attempts) {
            return fail(saved.error.empty() ? "Safe transient retry limit reached" : saved.error);
        }
    }
    return fail("Attempt limit reached");
}
} // namespace ltm
