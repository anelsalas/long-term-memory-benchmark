#include "long_term_memory_benchmark/dashboard_server.hpp"
#include <boost/asio/connect.hpp>
#include <boost/asio/ip/tcp.hpp>
#include <boost/beast/core.hpp>
#include <boost/beast/http.hpp>
#include <chrono>
#include <filesystem>
#include <fstream>
#include <functional>
#include <iostream>
#include <stdexcept>
#include <string>
#include <thread>
#include <utility>
#include <vector>

namespace {
using namespace ltm::dashboard;
namespace http = boost::beast::http;
using tcp = boost::asio::ip::tcp;

void require(bool condition, const std::string& message) {
    if (!condition) throw std::runtime_error(message);
}
struct TempDir {
    std::filesystem::path path = std::filesystem::temp_directory_path() / ("ltm-server-" + std::to_string(
        std::chrono::steady_clock::now().time_since_epoch().count()));
    TempDir() { std::filesystem::create_directories(path / "web"); }
    ~TempDir() { std::error_code ignored; std::filesystem::remove_all(path, ignored); }
};
struct NoLauncher : Launcher {
    std::unique_ptr<Process> start(const std::vector<std::string>&, const std::filesystem::path&) override {
        throw std::logic_error("No runs in server tests");
    }
};
// A live server on a free loopback port, running on its own thread.
struct LiveServer {
    TempDir dir;
    NoLauncher launcher;
    RunControl runs{launcher, dir.path / "runs", {"unused.exe"}};
    Api api{dir.path, runs, dir.path / "web"};
    Server server{api, "127.0.0.1", 0};
    std::thread thread{[this] { server.run(); }};
    LiveServer() { std::ofstream(dir.path / "web" / "index.html") << "<!doctype html><title>Dashboard</title>"; }
    ~LiveServer() { server.stop(); thread.join(); }

    http::response<http::string_body> request(http::verb verb, const std::string& target, const std::string& host,
            const std::string& body = "") {
        boost::asio::io_context io;
        tcp::socket socket(io);
        socket.connect({boost::asio::ip::make_address("127.0.0.1"), server.port()});
        http::request<http::string_body> req{verb, target, 11};
        req.set(http::field::host, host);
        if (!body.empty()) { req.set(http::field::content_type, "application/json"); req.body() = body; }
        req.prepare_payload();
        http::write(socket, req);
        boost::beast::flat_buffer buffer;
        http::response<http::string_body> res;
        http::read(socket, buffer, res);
        return res;
    }
    std::string own_host() const { return "127.0.0.1:" + std::to_string(server.port()); }
};

void round_trip() {
    LiveServer live;
    require(live.server.port() != 0, "Server reports the port it bound");
    auto res = live.request(http::verb::get, "/api/settings", live.own_host());
    require(res.result_int() == 200 && res.body().find("\"settings\":null") != std::string::npos, "API answers over HTTP");
    require(res[http::field::content_type] == "application/json", "JSON content type is sent");
    require(res[http::field::cache_control] == "no-store", "Responses are not cached");
    res = live.request(http::verb::get, "/", "localhost:" + std::to_string(live.server.port()));
    require(res.result_int() == 200 && res.body().find("Dashboard") != std::string::npos, "Page is served via localhost");
}
void foreign_host_refused() {
    LiveServer live;
    auto res = live.request(http::verb::get, "/api/settings", "evil.example:" + std::to_string(live.server.port()));
    require(res.result_int() == 403, "Requests for another host name are refused");
    res = live.request(http::verb::get, "/api/settings", "127.0.0.1:1");
    require(res.result_int() == 403, "Requests naming another port are refused");
}
}

int main(int argc, char** argv) {
    const std::vector<std::pair<std::string, std::function<void()>>> cases = {
        {"round_trip", round_trip}, {"foreign_host_refused", foreign_host_refused}};
    if (argc != 2) return 2;
    for (const auto& item : cases) {
        if (item.first == argv[1]) {
            try { item.second(); std::cout << "PASS " << item.first << '\n'; return 0; }
            catch (const std::exception& e) { std::cerr << "FAIL " << item.first << ": " << e.what() << '\n'; return 1; }
        }
    }
    return 2;
}
