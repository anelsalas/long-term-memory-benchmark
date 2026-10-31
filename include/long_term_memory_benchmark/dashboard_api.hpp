#pragma once
#include "long_term_memory_benchmark/dashboard_runs.hpp"
#include <filesystem>
#include <string>

namespace ltm::dashboard {
struct Request {
    std::string method;
    std::string target;
    std::string content_type;
    std::string body;
};
struct Response {
    int status = 200;
    std::string content_type = "application/json";
    std::string body;
};

// Maps dashboard HTTP requests onto settings and run control. No sockets here.
class Api {
  public:
    Api(std::filesystem::path data_dir, RunControl& runs, std::filesystem::path web_dir);
    Response handle(const Request& request);
  private:
    Response page(const Request& request) const;
    std::filesystem::path settings_file_;
    RunControl& runs_;
    std::filesystem::path web_dir_;
};
}
