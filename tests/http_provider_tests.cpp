#include "long_term_memory_benchmark/http_provider.hpp"
#include <functional>
#include <iostream>
#include <nlohmann/json.hpp>
#include <stdexcept>

namespace {
using Json = nlohmann::json;
void expect(bool condition, const char* message) {
    if (!condition) {
        throw std::runtime_error(message);
    }
}
struct StubTransport : ltm::HttpTransport {
    ltm::HttpResponse response;
    ltm::HttpRequest observed;
    unsigned calls = 0;
    ltm::HttpResponse post(const ltm::HttpRequest& request, const ltm::RequestControl&) override {
        ++calls;
        observed = request;
        return response;
    }
};
ltm::ProviderRequest request() {
    ltm::ProviderRequest result;
    result.case_id = "case";
    result.dispatch_id = "dispatch";
    result.model = {"stub", "family", "snapshot-v1"};
    result.prompt = "question\n\"evidence\"";
    result.output_token_limit = 10;
    result.response_byte_limit = 4096;
    return result;
}
ltm::ChatCompletionsOptions options() {
    return {"https://provider.example/v1/chat/completions", "max_tokens", 0.0, 1000};
}
Json successful_body() {
    return {
        {"id", "reply-1"},
        {"model", "snapshot-v1"},
        {"choices", Json::array({{{"finish_reason", "stop"}, {"message", {{"content", "91"}}}}})},
        {"usage", {{"prompt_tokens", 12}, {"completion_tokens", 3}, {"total_tokens", 15}}}};
}
void payload() {
    const auto body = Json::parse(ltm::chat_completion_body(request(), options()));
    expect(body.at("model") == "snapshot-v1", "Exact model version on wire");
    expect(body.at("messages").at(0).at("content") == request().prompt, "Prompt preserved");
    expect(body.at("max_tokens") == 10 && body.at("n") == 1 && body.at("stream") == false,
           "Bounded one-answer payload");
    auto selected = options();
    selected.output_token_parameter = "max_completion_tokens";
    expect(
        Json::parse(ltm::chat_completion_body(request(), selected)).at("max_completion_tokens") ==
            10,
        "Selectable completion cap");
}
void endpoint_validation() {
    for (const auto& endpoint :
         {"http://provider.example/v1", "https://user:password@provider.example/v1",
          "https://provider.example/v1?key=value", "https://provider.example/v1#fragment",
          "https://provider.example/\r\nheader"}) {
        bool rejected = false;
        try {
            ltm::validate_https_endpoint(endpoint);
        } catch (const std::invalid_argument&) {
            rejected = true;
        }
        expect(rejected, "Reject insecure or credential-bearing endpoint");
    }
    ltm::validate_https_endpoint(options().endpoint_url);
}
void successful_response() {
    StubTransport transport;
    transport.response = {200, successful_body().dump(), "header-id", ltm::HttpFailure::none};
    ltm::ChatCompletionsProvider provider(options(), "fixture-token", transport);
    const auto result = provider.generate(request(), {});
    expect(result.status == ltm::ProviderStatus::success && result.text == "91", "Parse answer");
    expect(result.usage && result.usage->input_tokens == 12 && result.usage->output_tokens == 3,
           "Parse billable usage");
    expect(result.request_id == "reply-1" && transport.calls == 1,
           "Record response ID and one dispatch");
    expect(provider.response_metadata_json().find("fixture-token") == std::string::npos,
           "Credentials absent from metadata");
}
void conservative_failures() {
    for (unsigned status : {301, 429, 500, 503}) {
        StubTransport transport;
        transport.response = {status, "sensitive error text", "", ltm::HttpFailure::none};
        ltm::ChatCompletionsProvider provider(options(), "fixture-token", transport);
        auto result = provider.generate(request(), {});
        expect(result.status == ltm::ProviderStatus::uncertain && transport.calls == 1,
               "No blind HTTP retry");
        expect(result.error.find("sensitive") == std::string::npos, "Sanitized provider error");
    }
    StubTransport transport;
    transport.response = {401, "", "", ltm::HttpFailure::none};
    ltm::ChatCompletionsProvider provider(options(), "fixture-token", transport);
    expect(provider.generate(request(), {}).status == ltm::ProviderStatus::authentication_error,
           "Classify authentication failure");
}
void malformed_or_wrong_model() {
    for (int mode = 0; mode < 4; ++mode) {
        auto body = successful_body();
        if (mode == 0) {
            body["model"] = "different";
        }
        if (mode == 1) {
            body["choices"][0]["finish_reason"] = "length";
        }
        if (mode == 2) {
            body["usage"]["completion_tokens"] = -1;
        }
        StubTransport transport;
        transport.response = {200, mode == 3 ? "invalid-json" : body.dump(), "",
                              ltm::HttpFailure::none};
        ltm::ChatCompletionsProvider provider(options(), "fixture-token", transport);
        expect(provider.generate(request(), {}).status != ltm::ProviderStatus::success,
               "Reject unverifiable response");
    }
}
void cancellation_and_transport() {
    StubTransport transport;
    ltm::ChatCompletionsProvider provider(options(), "fixture-token", transport);
    ltm::RequestControl control;
    control.cancelled = [] { return true; };
    expect(provider.generate(request(), control).status == ltm::ProviderStatus::cancelled &&
               transport.calls == 0,
           "Cancellation before transport");
    transport.response.failure = ltm::HttpFailure::possibly_sent;
    expect(provider.generate(request(), {}).status == ltm::ProviderStatus::uncertain,
           "Transport ambiguity retained");
}
} // namespace
int main() {
    const std::vector<std::pair<const char*, std::function<void()>>> tests = {
        {"payload", payload},
        {"endpoint_validation", endpoint_validation},
        {"successful_response", successful_response},
        {"conservative_failures", conservative_failures},
        {"malformed_or_wrong_model", malformed_or_wrong_model},
        {"cancellation_and_transport", cancellation_and_transport}};
    int failures = 0;
    for (const auto& test : tests) {
        try {
            test.second();
            std::cout << "PASS " << test.first << '\n';
        } catch (const std::exception& e) {
            ++failures;
            std::cerr << "FAIL " << test.first << ": " << e.what() << '\n';
        }
    }
    return failures ? 1 : 0;
}
