#include "long_term_memory_benchmark/memory_service_http.hpp"
#include <boost/asio.hpp>
#include <boost/beast.hpp>
#include <nlohmann/json.hpp>
#include <algorithm>
#include <stdexcept>

namespace ltm::retrieval {
namespace {
namespace asio = boost::asio;
namespace beast = boost::beast;
namespace http = beast::http;
using Json = nlohmann::json;
}

HttpMemoryService::HttpMemoryService(const std::string& base_url, std::chrono::milliseconds timeout)
    : timeout_(timeout) {
    constexpr auto scheme = "http://";
    if (base_url.rfind(scheme, 0) != 0) throw std::invalid_argument("Memory service must use local HTTP");
    const auto authority = base_url.substr(7);
    const auto colon = authority.rfind(':');
    if (colon == std::string::npos || authority.find('/') != std::string::npos)
        throw std::invalid_argument("Memory service requires a local host and port");
    host_ = authority.substr(0, colon);
    port_ = authority.substr(colon + 1);
    if ((host_ != "127.0.0.1" && host_ != "localhost") || port_.empty() || port_.size() > 5 ||
        !std::all_of(port_.begin(), port_.end(), [](char c) { return c >= '0' && c <= '9'; }) ||
        std::stoul(port_) == 0 || std::stoul(port_) > 65535 || timeout_.count() <= 0)
        throw std::invalid_argument("Memory service requires a local host and valid port");
}

std::string HttpMemoryService::request(const std::string& target, const std::string& body, bool post) {
    asio::io_context io;
    beast::tcp_stream stream(io);
    stream.expires_after(timeout_);
    stream.connect(asio::ip::tcp::resolver(io).resolve(host_ == "localhost" ? "127.0.0.1" : host_, port_));
    http::request<http::string_body> request{post ? http::verb::post : http::verb::get, target, 11};
    request.set(http::field::host, host_ + ":" + port_);
    request.set(http::field::content_type, "application/json");
    request.body() = body;
    request.prepare_payload();
    http::write(stream, request);
    beast::flat_buffer buffer;
    http::response_parser<http::string_body> parser;
    parser.body_limit(16u * 1024u * 1024u);
    http::read(stream, buffer, parser);
    if (parser.get().result() != http::status::ok)
        throw std::runtime_error("Memory service returned HTTP " + std::to_string(parser.get().result_int()));
    return parser.get().body();
}

std::string HttpMemoryService::config_json() {
    const auto body = request("/config", "", false);
    const auto config = Json::parse(body, nullptr, false);
    if (!config.is_object()) throw std::runtime_error("Invalid memory service configuration");
    return config.dump();
}

std::vector<MemoryRecord> HttpMemoryService::search(const std::string& name, const std::string& question,
    std::size_t limit) {
    if (name.empty() || question.empty() || !limit || limit > 100) throw std::invalid_argument("Invalid memory search");
    const auto body = request("/search", Json{{"namespace", name}, {"query", question}, {"limit", limit}}.dump(), true);
    const auto records = Json::parse(body, nullptr, false);
    if (!records.is_array() || records.size() > limit) throw std::runtime_error("Invalid memory search response");
    std::vector<MemoryRecord> result;
    for (const auto& record : records) {
        if (!record.is_object() || !record.contains("memory_id") || !record.at("memory_id").is_string() ||
            !record.contains("text") || !record.at("text").is_string())
            throw std::runtime_error("Invalid memory record");
        MemoryRecord item;
        item.id = record.at("memory_id").get<std::string>();
        item.text = record.at("text").get<std::string>();
        if (record.contains("metadata") && record.at("metadata").is_object()) {
            const auto& metadata = record.at("metadata");
            if (metadata.contains("session_date") && metadata.at("session_date").is_string())
                item.session_date = metadata.at("session_date").get<std::string>();
            if (metadata.contains("session_ids") && metadata.at("session_ids").is_array()) {
                for (const auto& id : metadata.at("session_ids"))
                    if (id.is_string()) item.source_sessions.push_back(id.get<std::string>());
            } else if (metadata.contains("session_id") && metadata.at("session_id").is_string()) {
                item.source_sessions.push_back(metadata.at("session_id").get<std::string>());
            }
        }
        result.push_back(std::move(item));
    }
    return result;
}

} // namespace ltm::retrieval
