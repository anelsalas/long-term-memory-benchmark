#include "long_term_memory_benchmark/embeddings_http.hpp"
#include <boost/asio.hpp>
#include <boost/beast.hpp>
#include <nlohmann/json.hpp>
#include <atomic>
#include <functional>
#include <iostream>
#include <stdexcept>
#include <string>
#include <thread>
#include <utility>
#include <vector>

namespace {
namespace asio = boost::asio;
namespace http = boost::beast::http;
using ltm::retrieval::HttpEmbedder;
using Json = nlohmann::json;
using Vectors = std::vector<std::vector<float>>;

void require(bool condition, const std::string& message) {
    if (!condition) throw std::runtime_error(message);
}

// Answers each request with one vector per input: {input length, request number}.
class FakeEndpoint {
public:
    explicit FakeEndpoint(http::status status = http::status::ok) : status_(status) {
        acceptor_.open(asio::ip::tcp::v4());
        acceptor_.bind({asio::ip::make_address("127.0.0.1"), 0});
        acceptor_.listen();
        thread_ = std::thread([this] { serve(); });
    }
    ~FakeEndpoint() {
        boost::system::error_code ignored;
        acceptor_.close(ignored);
        thread_.join();
    }
    std::string url() const { return "http://127.0.0.1:" + std::to_string(acceptor_.local_endpoint().port()) + "/v1/embeddings"; }
    std::vector<Json> requests;
private:
    void serve() {
        for (;;) {
            asio::ip::tcp::socket socket(io_);
            boost::system::error_code error;
            acceptor_.accept(socket, error);
            if (error) return;
            boost::beast::flat_buffer buffer;
            http::request<http::string_body> request;
            http::read(socket, buffer, request, error);
            if (error) continue;
            const auto body = Json::parse(request.body());
            requests.push_back(body);
            Json data = Json::array();
            for (std::size_t i = 0; i < body["input"].size(); ++i)
                data.push_back({{"index", i}, {"embedding", {body["input"][i].get<std::string>().size(), requests.size()}}});
            http::response<http::string_body> response{status_, request.version()};
            response.set(http::field::content_type, "application/json");
            response.body() = status_ == http::status::ok ? Json{{"data", data}}.dump() : R"({"error":"down"})";
            response.prepare_payload();
            http::write(socket, response, error);
        }
    }
    http::status status_;
    asio::io_context io_;
    asio::ip::tcp::acceptor acceptor_{io_};
    std::thread thread_;
};

void posts_batches_to_loopback() {
    FakeEndpoint endpoint;
    HttpEmbedder embedder(endpoint.url(), "embed-1", {2});
    require(embedder.embed({"a", "bbb", "cc"}) == Vectors{{1, 1}, {3, 1}, {2, 2}}, "Vectors follow input order across batches");
    require(endpoint.requests.size() == 2, "Inputs are sent in batches");
    require(endpoint.requests[0]["model"] == "embed-1" && endpoint.requests[1]["input"] == Json{"cc"}, "Batch contents");
    require(embedder.embed({}).empty() && endpoint.requests.size() == 2, "No texts, no request");
}
void accepts_only_loopback() {
    for (const std::string bad : {"https://127.0.0.1:8771/v1/embeddings", "http://example.com/v1/embeddings",
            "http://127.0.0.1.evil.example/v1", "http://127.0.0.1:notaport/v1", "127.0.0.1:8771"}) {
        bool refused = false;
        try { HttpEmbedder(bad, "m"); } catch (const std::invalid_argument&) { refused = true; }
        require(refused, "Must refuse " + bad);
    }
    HttpEmbedder("http://localhost:8771/v1/embeddings", "m");
    HttpEmbedder("http://[::1]:8771/v1/embeddings", "m");
}
void reports_http_errors() {
    FakeEndpoint endpoint(http::status::internal_server_error);
    bool failed = false;
    try { HttpEmbedder(endpoint.url(), "m").embed({"a"}); }
    catch (const std::runtime_error& e) { failed = std::string(e.what()).find("500") != std::string::npos; }
    require(failed, "An HTTP error is reported with its status");
}
void batches_by_size_and_cuts_long_texts() {
    FakeEndpoint endpoint;
    HttpEmbedder embedder(endpoint.url(), "m", {32, 5, 12});
    const std::string long_text = std::string(11, 'a') + "\xC3\xA9" + "bbbb";  // é spans bytes 12-13
    const auto vectors = embedder.embed({"abcdefgh", "abcdefgh", "abcdefghijkl", long_text});
    require(endpoint.requests.size() == 3, "A request holds at most 5 estimated tokens");
    require(endpoint.requests[0]["input"].size() == 2 && endpoint.requests[1]["input"].size() == 1, "Batches split by size");
    require(endpoint.requests[2]["input"][0] == std::string(11, 'a'), "Cut before the split character");
    require(vectors.size() == 4 && vectors[3] == std::vector<float>{11, 3} && embedder.truncated() == 1, "One cut text");
}
}

int main(int argc, char** argv) {
    const std::vector<std::pair<std::string, std::function<void()>>> cases = {
        {"posts_batches_to_loopback", posts_batches_to_loopback}, {"accepts_only_loopback", accepts_only_loopback},
        {"reports_http_errors", reports_http_errors},
        {"batches_by_size_and_cuts_long_texts", batches_by_size_and_cuts_long_texts}};
    if (argc != 2) return 2;
    for (const auto& item : cases) {
        if (item.first == argv[1]) {
            try { item.second(); std::cout << "PASS " << item.first << '\n'; return 0; }
            catch (const std::exception& e) { std::cerr << "FAIL " << item.first << ": " << e.what() << '\n'; return 1; }
        }
    }
    return 2;
}
