#include "long_term_memory_benchmark/embeddings_http.hpp"

#include "long_term_memory_benchmark/embeddings.hpp"
#include <boost/asio.hpp>
#include <boost/beast.hpp>
#include <algorithm>
#include <stdexcept>
#include <utility>

namespace ltm::retrieval {
namespace {
namespace asio = boost::asio;
namespace beast = boost::beast;
namespace http = beast::http;

bool digits(const std::string& s) {
    return !s.empty() && s.size() <= 5 && std::all_of(s.begin(), s.end(), [](char c) { return c >= '0' && c <= '9'; });
}
}

HttpEmbedder::HttpEmbedder(const std::string& url, std::string model, EmbeddingLimits limits,
    std::chrono::milliseconds timeout)
    : model_(std::move(model)), limits_(limits), timeout_(timeout) {
    const std::string scheme = "http://";
    if (url.compare(0, scheme.size(), scheme) != 0) throw std::invalid_argument("Embeddings URL must be http on this machine");
    const auto rest = url.substr(scheme.size());
    const auto slash = rest.find('/');
    const auto authority = rest.substr(0, slash);
    target_ = slash == std::string::npos ? "/" : rest.substr(slash);
    const auto colon = authority.rfind(':');
    host_ = authority.substr(0, colon);
    port_ = colon == std::string::npos ? "80" : authority.substr(colon + 1);
    if (colon != std::string::npos && host_.find(']') == std::string::npos && host_.find(':') != std::string::npos)
        throw std::invalid_argument("Embeddings URL host is invalid");
    if (host_ == "[::1]") host_ = "::1";
    if ((host_ != "127.0.0.1" && host_ != "localhost" && host_ != "::1") || !digits(port_))
        throw std::invalid_argument("Embeddings URL must be http://127.0.0.1, localhost or [::1] with a numeric port");
    if (limits_.batch_texts == 0 || limits_.batch_tokens == 0 || limits_.text_bytes == 0)
        throw std::invalid_argument("Embedding limits must be positive");
}

std::vector<std::vector<float>> HttpEmbedder::embed(const std::vector<std::string>& texts) {
    std::vector<std::vector<float>> vectors;
    std::vector<std::string> batch;
    std::size_t batch_tokens = 0;
    const auto send = [&] {
        for (auto& vector : post(batch)) vectors.push_back(std::move(vector));
        batch.clear();
        batch_tokens = 0;
    };
    for (const auto& text : texts) {
        auto input = text;
        if (input.size() > limits_.text_bytes) {
            auto cut = limits_.text_bytes;
            while (cut > 0 && (static_cast<unsigned char>(input[cut]) & 0xC0) == 0x80) --cut;
            input.resize(cut);
            ++truncated_;
        }
        const auto tokens = estimated_tokens(input.size());
        if (!batch.empty() && (batch.size() == limits_.batch_texts || batch_tokens + tokens > limits_.batch_tokens)) send();
        batch.push_back(std::move(input));
        batch_tokens += tokens;
    }
    if (!batch.empty()) send();
    return vectors;
}

std::vector<std::vector<float>> HttpEmbedder::post(const std::vector<std::string>& batch) {
    std::vector<std::vector<float>> vectors;
    {
        asio::io_context io;
        beast::tcp_stream stream(io);
        stream.expires_after(timeout_);
        stream.connect(asio::ip::tcp::resolver(io).resolve(host_, port_));
        http::request<http::string_body> request{http::verb::post, target_, 11};
        request.set(http::field::host, host_ + ":" + port_);
        request.set(http::field::content_type, "application/json");
        request.body() = embedding_request(model_, batch);
        request.prepare_payload();
        http::write(stream, request);
        beast::flat_buffer buffer;
        http::response_parser<http::string_body> parser;
        parser.body_limit(256u * 1024u * 1024u);
        http::read(stream, buffer, parser);
        const auto& response = parser.get();
        if (response.result() != http::status::ok)
            throw std::runtime_error("Embeddings endpoint returned HTTP " + std::to_string(response.result_int()));
        for (auto& vector : parse_embeddings(response.body(), batch.size())) vectors.push_back(std::move(vector));
        beast::error_code ignored;
        stream.socket().shutdown(asio::ip::tcp::socket::shutdown_both, ignored);
    }
    return vectors;
}

} // namespace ltm::retrieval
