#include "long_term_memory_benchmark/memory_service_http.hpp"
#include <boost/asio.hpp>
#include <boost/beast.hpp>
#include <nlohmann/json.hpp>
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
using Json = nlohmann::json;
using ltm::retrieval::HttpMemoryService;

void require(bool condition, const std::string& message) {
    if (!condition) throw std::runtime_error(message);
}
class FakeEndpoint {
public:
    explicit FakeEndpoint(std::string body) : body_(std::move(body)) {
        acceptor_.open(asio::ip::tcp::v4());
        acceptor_.bind({asio::ip::make_address("127.0.0.1"), 0});
        acceptor_.listen();
        port_ = acceptor_.local_endpoint().port();
        thread_ = std::thread([this] {
            asio::ip::tcp::socket socket(io_);
            boost::system::error_code error;
            acceptor_.accept(socket, error);
            if (error) return;
            boost::beast::flat_buffer buffer;
            http::request<http::string_body> request;
            http::read(socket, buffer, request, error);
            if (error) return;
            target = std::string(request.target());
            request_body = request.body();
            http::response<http::string_body> response{http::status::ok, request.version()};
            response.set(http::field::content_type, "application/json");
            response.body() = body_;
            response.prepare_payload();
            http::write(socket, response, error);
        });
    }
    ~FakeEndpoint() {
        boost::system::error_code ignored;
        acceptor_.close(ignored);
        if (thread_.joinable()) thread_.join();
    }
    std::string url() const { return "http://127.0.0.1:" + std::to_string(port_); }
    void wait() { if (thread_.joinable()) thread_.join(); }
    std::string target, request_body;
private:
    std::string body_;
    unsigned short port_ = 0;
    asio::io_context io_;
    asio::ip::tcp::acceptor acceptor_{io_};
    std::thread thread_;
};

void searches_local_memory_and_preserves_metadata() {
    FakeEndpoint endpoint(R"([{"memory_id":"m1","text":"grade 82","score":0.8,
      "metadata":{"session_id":"s1","session_date":"2026-03-02T10:00:00Z"}},
      {"memory_id":"m2","text":"no provenance","metadata":null}])");
    HttpMemoryService service(endpoint.url());
    const auto memories = service.search("case1", "grade?", 5);
    endpoint.wait();
    require(memories.size() == 2 && memories[0].id == "m1" && memories[0].source_sessions == std::vector<std::string>{"s1"},
        "Memory and source-session metadata round trip");
    require(memories[0].session_date == "2026-03-02T10:00:00Z" && memories[1].source_sessions.empty(),
        "Date and missing provenance remain distinct");
    const auto request = Json::parse(endpoint.request_body);
    require(endpoint.target == "/search" && request["namespace"] == "case1" && request["query"] == "grade?" &&
        request["limit"] == 5, "Search request matches sidecar contract");
}
void refuses_nonlocal_services() {
    for (const std::string bad : {"https://127.0.0.1:8774", "http://example.com:8774", "http://127.0.0.1.evil:8774",
                                  "http://127.0.0.1:99999", "http://127.0.0.1:8774/path"}) {
        bool rejected = false;
        try { HttpMemoryService service(bad); } catch (const std::invalid_argument&) { rejected = true; }
        require(rejected, "Foreign or invalid service URL is refused");
    }
}
}

int main(int argc, char** argv) {
    const std::vector<std::pair<std::string, std::function<void()>>> cases = {
        {"searches_local_memory_and_preserves_metadata", searches_local_memory_and_preserves_metadata},
        {"refuses_nonlocal_services", refuses_nonlocal_services}};
    if (argc != 2) return 2;
    for (const auto& item : cases) if (item.first == argv[1]) {
        try { item.second(); std::cout << "PASS " << item.first << '\n'; return 0; }
        catch (const std::exception& e) { std::cerr << "FAIL " << item.first << ": " << e.what() << '\n'; return 1; }
    }
    return 2;
}
