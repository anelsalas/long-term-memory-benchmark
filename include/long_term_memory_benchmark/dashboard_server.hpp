#pragma once
#include "long_term_memory_benchmark/dashboard_api.hpp"
#include <memory>
#include <string>

namespace ltm::dashboard {
// Serves an Api over HTTP/1.1. Every request is handled on the thread that calls run(),
// so the Api and run control are only ever used from one thread.
class Server {
  public:
    Server(Api& api, const std::string& address, unsigned short port);
    ~Server();
    unsigned short port() const;
    void run();
    void stop();
  private:
    struct State;
    std::unique_ptr<State> state_;
};
}
