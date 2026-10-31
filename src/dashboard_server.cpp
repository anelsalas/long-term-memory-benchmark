#include "long_term_memory_benchmark/dashboard_server.hpp"
#include <boost/asio/ip/tcp.hpp>
#include <boost/asio/post.hpp>
#include <boost/beast/core.hpp>
#include <boost/beast/http.hpp>
#include <chrono>
#include <optional>
#include <set>

namespace ltm::dashboard {
namespace {
namespace beast = boost::beast;
namespace http = beast::http;
using tcp = boost::asio::ip::tcp;

constexpr std::size_t body_limit = 1024 * 1024;
constexpr auto read_timeout = std::chrono::seconds(15);

http::response<http::string_body> reply(const Response& r, unsigned version, bool keep_alive) {
    http::response<http::string_body> res{static_cast<http::status>(r.status), version};
    res.set(http::field::content_type, r.content_type);
    res.set(http::field::cache_control, "no-store");
    res.set("X-Content-Type-Options", "nosniff");
    res.set(http::field::content_security_policy, "default-src 'self'; frame-ancestors 'none'");
    res.keep_alive(keep_alive);
    res.body() = r.body;
    res.prepare_payload();
    return res;
}

class Session : public std::enable_shared_from_this<Session> {
  public:
    Session(tcp::socket socket, Api& api, const std::set<std::string>& hosts)
        : stream_(std::move(socket)), api_(api), hosts_(hosts) {}
    void read() {
        parser_.emplace();
        parser_->body_limit(body_limit);
        stream_.expires_after(read_timeout);
        http::async_read(stream_, buffer_, *parser_,
            [self = shared_from_this()](beast::error_code ec, std::size_t) { self->on_read(ec); });
    }
  private:
    void on_read(beast::error_code ec) {
        if (ec == http::error::body_limit) return write(reply({413, "application/json", R"({"error":"Request too large"})"}, 11, false));
        if (ec) return close();
        const auto req = parser_->release();
        const std::string host(req[http::field::host]);
        Response response;
        if (!hosts_.count(host)) response = {403, "application/json", R"({"error":"Unexpected Host header"})"};
        else response = api_.handle({std::string(req.method_string()), std::string(req.target()),
            std::string(req[http::field::content_type]), req.body()});
        write(reply(response, req.version(), req.keep_alive()));
    }
    void write(http::response<http::string_body> res) {
        response_ = std::move(res);
        http::async_write(stream_, *response_, [self = shared_from_this()](beast::error_code ec, std::size_t) {
            if (ec || !self->response_->keep_alive()) return self->close();
            self->read();
        });
    }
    void close() {
        beast::error_code ignored;
        stream_.socket().shutdown(tcp::socket::shutdown_both, ignored);
    }
    beast::tcp_stream stream_;
    beast::flat_buffer buffer_;
    std::optional<http::request_parser<http::string_body>> parser_;
    std::optional<http::response<http::string_body>> response_;
    Api& api_;
    const std::set<std::string>& hosts_;
};
}

struct Server::State {
    Api& api;
    boost::asio::io_context io;
    tcp::acceptor acceptor;
    std::set<std::string> hosts;
    State(Api& a, const std::string& address, unsigned short port)
        : api(a), acceptor(io, {boost::asio::ip::make_address(address), port}) {
        const auto bound = std::to_string(acceptor.local_endpoint().port());
        hosts = {"localhost:" + bound, "127.0.0.1:" + bound, "[::1]:" + bound};
        const auto ip = acceptor.local_endpoint().address();
        hosts.insert((ip.is_v6() ? "[" + ip.to_string() + "]" : ip.to_string()) + ":" + bound);
    }
    void accept() {
        acceptor.async_accept([this](beast::error_code ec, tcp::socket socket) {
            if (ec) return;
            std::make_shared<Session>(std::move(socket), api, hosts)->read();
            accept();
        });
    }
};

Server::Server(Api& api, const std::string& address, unsigned short port)
    : state_(std::make_unique<State>(api, address, port)) {}
Server::~Server() = default;
unsigned short Server::port() const { return state_->acceptor.local_endpoint().port(); }
void Server::run() {
    state_->accept();
    state_->io.run();
}
void Server::stop() {
    boost::asio::post(state_->io, [this] {
        beast::error_code ignored;
        state_->acceptor.close(ignored);
        state_->io.stop();
    });
}
}
