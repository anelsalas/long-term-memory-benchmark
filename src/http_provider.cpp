#include "long_term_memory_benchmark/http_provider.hpp"
#include <algorithm>
#include <array>
#include <cmath>
#include <limits>
#include <memory>
#include <nlohmann/json.hpp>
#include <set>
#include <stdexcept>
#include <utility>
#define NOMINMAX
#include <windows.h>
#include <winhttp.h>

namespace ltm {
namespace {
using Json = nlohmann::json;
struct InternetCloser {
    void operator()(void* handle) const {
        if (handle) {
            WinHttpCloseHandle(handle);
        }
    }
};
using InternetHandle = std::unique_ptr<void, InternetCloser>;
std::wstring wide(const std::string& text) {
    return {text.begin(), text.end()};
}
struct Endpoint {
    std::wstring host, path;
    INTERNET_PORT port;
};
Endpoint split_endpoint(const std::string& endpoint) {
    if (endpoint.empty() || endpoint.size() > 4096 ||
        std::any_of(endpoint.begin(), endpoint.end(),
                    [](unsigned char c) { return c <= 32 || c >= 127; }) ||
        endpoint.find('?') != std::string::npos || endpoint.find('#') != std::string::npos ||
        endpoint.find('@') != std::string::npos) {
        throw std::invalid_argument(
            "HTTPS endpoint must contain an ASCII host/path without credentials or query");
    }
    auto url = wide(endpoint);
    URL_COMPONENTS parts{};
    parts.dwStructSize = sizeof(parts);
    parts.dwHostNameLength = static_cast<DWORD>(-1);
    parts.dwUrlPathLength = static_cast<DWORD>(-1);
    parts.dwUserNameLength = static_cast<DWORD>(-1);
    parts.dwPasswordLength = static_cast<DWORD>(-1);
    if (!WinHttpCrackUrl(url.c_str(), static_cast<DWORD>(url.size()), 0, &parts) ||
        parts.nScheme != INTERNET_SCHEME_HTTPS || !parts.dwHostNameLength || !parts.nPort ||
        parts.dwUserNameLength || parts.dwPasswordLength) {
        throw std::invalid_argument("A valid HTTPS endpoint is required");
    }
    return {{parts.lpszHostName, parts.dwHostNameLength},
            parts.dwUrlPathLength ? std::wstring(parts.lpszUrlPath, parts.dwUrlPathLength) : L"/",
            parts.nPort};
}
void validate_options(const ChatCompletionsOptions& options) {
    validate_https_endpoint(options.endpoint_url);
    if (options.output_token_parameter != "max_tokens" &&
        options.output_token_parameter != "max_completion_tokens") {
        throw std::invalid_argument("Select a supported output token parameter");
    }
    if (!options.timeout_ms ||
        options.timeout_ms > static_cast<unsigned>(std::numeric_limits<int>::max())) {
        throw std::invalid_argument("Timeout must be positive and representable");
    }
    if (options.temperature && (!std::isfinite(*options.temperature) || *options.temperature < 0 ||
                                *options.temperature > 2)) {
        throw std::invalid_argument("Temperature must be within zero and two");
    }
}
std::uint64_t token_count(const Json& value) {
    if (value.is_number_unsigned()) {
        return value.get<std::uint64_t>();
    }
    if (value.is_number_integer() && value.get<std::int64_t>() >= 0) {
        return static_cast<std::uint64_t>(value.get<std::int64_t>());
    }
    throw std::invalid_argument("Invalid response token count");
}
Json parse_response(const std::string& text) {
    std::vector<std::set<std::string>> keys;
    return Json::parse(text, [&](int, Json::parse_event_t event, Json& value) {
        if (event == Json::parse_event_t::object_start) {
            keys.emplace_back();
        } else if (event == Json::parse_event_t::key &&
                   !keys.back().insert(value.get<std::string>()).second) {
            throw std::invalid_argument("Duplicate response field");
        } else if (event == Json::parse_event_t::object_end) {
            keys.pop_back();
        }
        return true;
    });
}
std::string request_header(HINTERNET handle) {
    DWORD size = 0;
    WinHttpQueryHeaders(handle, WINHTTP_QUERY_CUSTOM, L"x-request-id", nullptr, &size, nullptr);
    if (!size || size > 4096) {
        return {};
    }
    std::vector<wchar_t> value(size / sizeof(wchar_t) + 1);
    if (!WinHttpQueryHeaders(handle, WINHTTP_QUERY_CUSTOM, L"x-request-id", value.data(), &size,
                             nullptr)) {
        return {};
    }
    const std::wstring header(value.data());
    if (std::any_of(header.begin(), header.end(), [](wchar_t c) { return c < 32 || c > 126; })) {
        return {};
    }
    std::string result;
    for (const auto character : header) {
        result.push_back(static_cast<char>(character));
    }
    return result;
}
} // namespace

void validate_https_endpoint(const std::string& endpoint) {
    (void)split_endpoint(endpoint);
}

std::string chat_completion_body(const ProviderRequest& request,
                                 const ChatCompletionsOptions& options) {
    validate_options(options);
    if (request.protocol_version != 1 || request.model.version.empty() ||
        !request.output_token_limit) {
        throw std::invalid_argument("Versioned request, model and output limit required");
    }
    Json body = {{"model", request.model.version},
                 {"messages", Json::array({{{"role", "user"}, {"content", request.prompt}}})},
                 {"stream", false},
                 {"n", 1},
                 {options.output_token_parameter, request.output_token_limit}};
    if (options.temperature) {
        body["temperature"] = *options.temperature;
    }
    return body.dump();
}

HttpResponse WinHttpTransport::post(const HttpRequest& request, const RequestControl& control) {
    HttpResponse response;
    response.failure = HttpFailure::before_send;
    const auto endpoint = split_endpoint(request.endpoint_url);
    if (!request.timeout_ms ||
        request.timeout_ms > static_cast<unsigned>(std::numeric_limits<int>::max()) ||
        !request.max_response_bytes || request.body.size() > std::numeric_limits<DWORD>::max()) {
        return response;
    }
    if (request.bearer_token.empty() ||
        std::any_of(request.bearer_token.begin(), request.bearer_token.end(),
                    [](unsigned char c) { return c <= 32 || c > 126; })) {
        return response;
    }
    const auto end = ProviderClock::now() + std::chrono::milliseconds(request.timeout_ms);
    auto remaining = [&]() {
        if (control.stop_requested()) {
            return 0;
        }
        auto time_left =
            std::chrono::duration_cast<std::chrono::milliseconds>(end - ProviderClock::now())
                .count();
        if (control.deadline) {
            time_left = std::min(time_left, std::chrono::duration_cast<std::chrono::milliseconds>(
                                                *control.deadline - control.now())
                                                .count());
        }
        return static_cast<int>(
            std::max<std::int64_t>(0, std::min<std::int64_t>(time_left, request.timeout_ms)));
    };
    InternetHandle session(WinHttpOpen(L"LongTermMemoryBenchmark/0.1",
                                       WINHTTP_ACCESS_TYPE_AUTOMATIC_PROXY, WINHTTP_NO_PROXY_NAME,
                                       WINHTTP_NO_PROXY_BYPASS, 0));
    if (!session || !remaining()) {
        return response;
    }
    DWORD redirects = WINHTTP_OPTION_REDIRECT_POLICY_NEVER;
    if (!WinHttpSetOption(session.get(), WINHTTP_OPTION_REDIRECT_POLICY, &redirects,
                          sizeof(redirects))) {
        return response;
    }
    InternetHandle connection(
        WinHttpConnect(session.get(), endpoint.host.c_str(), endpoint.port, 0));
    if (!connection) {
        return response;
    }
    InternetHandle handle(WinHttpOpenRequest(connection.get(), L"POST", endpoint.path.c_str(),
                                             nullptr, WINHTTP_NO_REFERER,
                                             WINHTTP_DEFAULT_ACCEPT_TYPES, WINHTTP_FLAG_SECURE));
    if (!handle) {
        return response;
    }
    DWORD autologon = WINHTTP_AUTOLOGON_SECURITY_LEVEL_HIGH;
    if (!WinHttpSetOption(handle.get(), WINHTTP_OPTION_AUTOLOGON_POLICY, &autologon,
                          sizeof(autologon))) {
        return response;
    }
    auto set_timeouts = [&]() {
        const int timeout = remaining();
        return timeout > 0 && WinHttpSetTimeouts(handle.get(), timeout, timeout, timeout, timeout);
    };
    if (!set_timeouts()) {
        return response;
    }
    const auto headers = wide("Content-Type: application/json\r\nAuthorization: Bearer " +
                              request.bearer_token + "\r\n");
    response.failure = HttpFailure::possibly_sent;
    if (!WinHttpSendRequest(handle.get(), headers.c_str(), static_cast<DWORD>(headers.size()),
                            const_cast<char*>(request.body.data()),
                            static_cast<DWORD>(request.body.size()),
                            static_cast<DWORD>(request.body.size()), 0)) {
        return response;
    }
    if (!set_timeouts() || !WinHttpReceiveResponse(handle.get(), nullptr)) {
        return response;
    }
    DWORD status = 0, size = sizeof(status);
    if (!WinHttpQueryHeaders(handle.get(), WINHTTP_QUERY_STATUS_CODE | WINHTTP_QUERY_FLAG_NUMBER,
                             WINHTTP_HEADER_NAME_BY_INDEX, &status, &size, nullptr)) {
        return response;
    }
    response.status_code = status;
    response.request_id = request_header(handle.get());
    if (status < 200 || status >= 300) {
        response.failure = HttpFailure::none;
        return response;
    }
    std::array<char, 4096> buffer{};
    for (;;) {
        if (!set_timeouts()) {
            return response;
        }
        DWORD bytes = 0;
        if (!WinHttpReadData(handle.get(), buffer.data(), static_cast<DWORD>(buffer.size()),
                             &bytes)) {
            return response;
        }
        if (!bytes) {
            break;
        }
        if (bytes > request.max_response_bytes - response.body.size()) {
            return response;
        }
        response.body.append(buffer.data(), bytes);
    }
    response.failure = HttpFailure::none;
    return response;
}

ChatCompletionsProvider::ChatCompletionsProvider(ChatCompletionsOptions options, std::string bearer,
                                                 HttpTransport& transport)
    : options_(std::move(options)), bearer_token_(std::move(bearer)), transport_(transport) {
    validate_options(options_);
    if (bearer_token_.empty() || std::any_of(bearer_token_.begin(), bearer_token_.end(),
                                             [](unsigned char c) { return c <= 32 || c > 126; })) {
        throw std::invalid_argument("A valid environment credential is required");
    }
}

ProviderReply ChatCompletionsProvider::generate(const ProviderRequest& request,
                                                const RequestControl& control) {
    metadata_ = "{}";
    if (control.stop_requested()) {
        return {ProviderStatus::cancelled, "", "", "Cancelled before HTTP transport", {}};
    }
    const auto body = chat_completion_body(request, options_);
    const auto response = transport_.post({options_.endpoint_url, bearer_token_, body,
                                           options_.timeout_ms, request.response_byte_limit},
                                          control);
    ProviderReply reply;
    reply.request_id = response.request_id;
    if (response.failure != HttpFailure::none) {
        reply.status = response.failure == HttpFailure::before_send
                           ? ProviderStatus::configuration_error
                           : ProviderStatus::uncertain;
        reply.error = "HTTP transport incomplete; inspect request identity before retrying";
        return reply;
    }
    if (response.status_code < 200 || response.status_code >= 300) {
        if (response.status_code == 401 || response.status_code == 403) {
            reply.status = ProviderStatus::authentication_error;
        } else if (response.status_code == 402) {
            reply.status = ProviderStatus::billing_error;
        } else if (response.status_code == 400 || response.status_code == 404 ||
                   response.status_code == 422) {
            reply.status = ProviderStatus::configuration_error;
        } else {
            reply.status = ProviderStatus::uncertain;
        }
        reply.error =
            "HTTP " + std::to_string(response.status_code) + "; provider response body withheld";
        return reply;
    }
    try {
        if (response.body.size() > request.response_byte_limit) {
            throw std::invalid_argument("Response cap");
        }
        const auto parsed = parse_response(response.body);
        Json metadata = {{"http_status", response.status_code},
                         {"http_request_id", response.request_id}};
        for (const auto* field : {"id", "model", "system_fingerprint", "created", "usage"}) {
            if (parsed.contains(field)) {
                metadata[field] = parsed.at(field);
            }
        }
        metadata_ = metadata.dump();
        if (parsed.contains("id")) {
            reply.request_id = parsed.at("id").get<std::string>();
        }
        if (parsed.contains("usage") && !parsed.at("usage").is_null()) {
            const auto& usage = parsed.at("usage");
            reply.usage = ProviderUsage{token_count(usage.at("prompt_tokens")),
                                        token_count(usage.at("completion_tokens"))};
        }
        if (parsed.at("model").get<std::string>() != request.model.version) {
            reply.status = ProviderStatus::configuration_error;
            reply.error = "Returned model differs from the declared exact version";
            return reply;
        }
        const auto& choices = parsed.at("choices");
        if (!choices.is_array() || choices.size() != 1 ||
            choices[0].at("finish_reason") != "stop") {
            throw std::invalid_argument("Incomplete answer");
        }
        reply.text = choices[0].at("message").at("content").get<std::string>();
        if (reply.text.empty()) {
            throw std::invalid_argument("Empty answer");
        }
        reply.status = ProviderStatus::success;
    } catch (...) {
        reply.status = ProviderStatus::uncertain;
        reply.text.clear();
        reply.error = "Incomplete or malformed chat response; reconcile submission";
    }
    return reply;
}
} // namespace ltm
