#pragma once

#include "provider.hpp"
#include <optional>
#include <string>

namespace ltm {
struct HttpRequest {
    std::string endpoint_url;
    std::string bearer_token;
    std::string body;
    std::uint32_t timeout_ms = 30000;
    std::uint64_t max_response_bytes = 1024 * 1024;
    bool local_mode = false;
};
enum class HttpFailure { none, before_send, possibly_sent };
struct HttpResponse {
    unsigned status_code = 0;
    std::string body;
    std::string request_id;
    HttpFailure failure = HttpFailure::none;
};
class HttpTransport {
  public:
    virtual ~HttpTransport() = default;
    virtual HttpResponse post(const HttpRequest& request, const RequestControl& control) = 0;
};
class WinHttpTransport final : public HttpTransport {
  public:
    HttpResponse post(const HttpRequest& request, const RequestControl& control) override;
};
struct ChatCompletionsOptions {
    std::string endpoint_url;
    std::string output_token_parameter = "max_completion_tokens";
    std::optional<double> temperature;
    std::uint32_t timeout_ms = 30000;
    bool local_mode = false;
};
void validate_https_endpoint(const std::string& endpoint);
void validate_chat_endpoint(const std::string& endpoint, bool local_mode);
std::string chat_completion_body(const ProviderRequest& request,
                                 const ChatCompletionsOptions& options);
class ChatCompletionsProvider final : public Provider {
  public:
    ChatCompletionsProvider(ChatCompletionsOptions options, std::string bearer_token,
                            HttpTransport& transport);
    ProviderReply generate(const ProviderRequest& request, const RequestControl& control) override;
    const std::string& response_metadata_json() const {
        return metadata_;
    }

  private:
    ChatCompletionsOptions options_;
    std::string bearer_token_;
    HttpTransport& transport_;
    std::string metadata_ = "{}";
};
} // namespace ltm
