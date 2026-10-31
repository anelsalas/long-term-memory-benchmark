#include "long_term_memory_benchmark/dashboard_api.hpp"
#include <nlohmann/json.hpp>
#include <algorithm>
#include <cctype>
#include <fstream>
#include <optional>
#include <sstream>
#include <stdexcept>

namespace ltm::dashboard {
namespace {
using Json = nlohmann::json;
Response json(int status, const Json& body) { return {status, "application/json", body.dump()}; }
Response error(int status, const std::string& message) { return json(status, {{"error", message}}); }
bool sent_as_json(const Request& request) { return request.content_type.rfind("application/json", 0) == 0; }
const char* status_name(RunStatus status) {
    switch (status) {
        case RunStatus::running: return "running";
        case RunStatus::completed: return "completed";
        case RunStatus::failed: return "failed";
        case RunStatus::stopped: return "stopped";
        case RunStatus::interrupted: return "interrupted";
    }
    return "unknown";
}
Json run_json(const RunInfo& run) {
    return {{"id", run.id}, {"status", status_name(run.status)},
        {"exit_code", run.exit_code ? Json(*run.exit_code) : Json(nullptr)}};
}
// "/api/runs/<id>/<action>" -> id, or empty when the target has another shape.
std::string run_id(const std::string& target, const std::string& action) {
    const std::string prefix = "/api/runs/", suffix = "/" + action;
    if (target.size() <= prefix.size() + suffix.size() || target.rfind(prefix, 0) != 0
        || target.compare(target.size() - suffix.size(), suffix.size(), suffix) != 0) return {};
    const auto id = target.substr(prefix.size(), target.size() - prefix.size() - suffix.size());
    return id.find('/') == std::string::npos ? id : std::string();
}
// Reads at most the last `limit` bytes, so a chatty run cannot flood the page.
std::string read_tail(const std::filesystem::path& file, std::uintmax_t limit) {
    std::ifstream in(file, std::ios::binary);
    if (!in) return {};
    in.seekg(0, std::ios::end);
    const auto size = static_cast<std::uintmax_t>(in.tellg());
    const auto start = size > limit ? size - limit : 0;
    in.seekg(static_cast<std::streamoff>(start));
    std::ostringstream text; text << in.rdbuf();
    return text.str();
}
// Page files are single plain names such as app.js; anything else is not served.
std::optional<std::string> page_type(const std::string& name) {
    const auto dot = name.rfind('.');
    if (dot == std::string::npos || dot == 0) return std::nullopt;
    const auto plain = std::all_of(name.begin(), name.begin() + static_cast<std::ptrdiff_t>(dot), [](unsigned char c) {
        return std::isalnum(c) || c == '-' || c == '_';
    });
    if (!plain) return std::nullopt;
    const auto extension = name.substr(dot);
    if (extension == ".html") return "text/html; charset=utf-8";
    if (extension == ".js") return "text/javascript; charset=utf-8";
    if (extension == ".css") return "text/css; charset=utf-8";
    return std::nullopt;
}
}

Api::Api(std::filesystem::path data_dir, RunControl& runs, std::filesystem::path web_dir)
    : settings_file_(std::move(data_dir) / "settings.json"), runs_(runs), web_dir_(std::move(web_dir)) {}

Response Api::handle(const Request& request) {
    if (request.method != "GET" && !sent_as_json(request)) return error(415, "Send changes as application/json");
    if (request.target == "/api/settings") {
        if (request.method == "PUT") {
            try {
                const auto settings = settings_from_json(request.body);
                save_settings(settings_file_, settings);
            } catch (const std::invalid_argument& e) { return error(400, e.what()); }
        } else if (request.method != "GET") return error(405, "Use GET or PUT");
        const auto saved = load_settings(settings_file_);
        return json(200, {{"settings", saved ? Json::parse(settings_to_json(*saved)) : Json(nullptr)}});
    }
    if (request.target == "/api/runs") {
        if (request.method == "GET") {
            auto list = Json::array();
            for (const auto& run : runs_.runs()) list.push_back(run_json(run));
            return json(200, {{"runs", list}});
        }
        if (request.method != "POST") return error(405, "Use GET or POST");
        const auto settings = load_settings(settings_file_);
        if (!settings) return error(409, "Save settings before starting a run");
        try {
            const auto id = runs_.start(*settings);
            for (const auto& run : runs_.runs())
                if (run.id == id) return json(201, {{"run", run_json(run)}});
            return error(500, "Started run is missing");
        } catch (const std::logic_error& e) { return error(409, e.what()); }
    }
    if (const auto id = run_id(request.target, "stop"); !id.empty()) {
        if (request.method != "POST") return error(405, "Use POST");
        try { runs_.stop(id); } catch (const std::invalid_argument&) { return error(404, "Unknown run"); }
        return json(200, {{"stopping", id}});
    }
    if (const auto id = run_id(request.target, "output"); !id.empty()) {
        if (request.method != "GET") return error(405, "Use GET");
        try {
            return {200, "text/plain; charset=utf-8", read_tail(runs_.output_file(id), 256 * 1024)};
        } catch (const std::invalid_argument&) { return error(404, "Unknown run"); }
    }
    if (request.target.rfind("/api/", 0) == 0) return error(404, "Not found");
    return page(request);
}
Response Api::page(const Request& request) const {
    if (request.method != "GET") return error(405, "Page files are read-only");
    auto name = request.target.substr(0, request.target.find('?'));
    if (name.empty() || name[0] != '/') return error(404, "Not found");
    name = name == "/" ? "index.html" : name.substr(1);
    const auto type = page_type(name);
    if (!type) return error(404, "Not found");
    std::ifstream in(web_dir_ / name, std::ios::binary);
    if (!in) return error(404, "Not found");
    std::ostringstream content; content << in.rdbuf();
    return {200, *type, content.str()};
}
}
