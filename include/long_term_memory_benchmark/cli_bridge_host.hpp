#pragma once

#include "long_term_memory_benchmark/cli_bridge.hpp"
#include <memory>

namespace ltm::cli_bridge {
// Runs each command in a kill-on-close Job Object. Standard input and output are
// pipes; standard error goes to NUL. The child inherits this process's environment,
// which is how each CLI finds its own sign-in.
class WindowsCommandRunner final : public CommandRunner {
  public:
    RunResult run(const Invocation& invocation) override;
};

// Serves POST /v1/chat/completions and GET /health on 127.0.0.1 only. Each
// connection gets its own thread and one request, so slow replies do not queue.
// Requests with an Origin header or a Host other than this loopback port are refused.
class Server {
  public:
    Server(Bridge& bridge, unsigned short port);
    ~Server();
    unsigned short port() const;
    void run();  // blocks until stop()
    void stop();

  private:
    struct State;
    std::unique_ptr<State> state_;
};
} // namespace ltm::cli_bridge
