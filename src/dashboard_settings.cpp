#include "long_term_memory_benchmark/dashboard_settings.hpp"
#include <nlohmann/json.hpp>
#include <algorithm>
#include <fstream>
#include <limits>
#include <sstream>
#include <set>
#include <stdexcept>
#include <vector>

namespace ltm::dashboard {
namespace {
using Json = nlohmann::json;
[[noreturn]] void invalid(const std::string& message) { throw std::invalid_argument(message); }
bool named(const std::string& s) { return std::any_of(s.begin(), s.end(), [](unsigned char c) { return c > 32; }); }

Json parse(const std::string& text) {
    std::vector<std::set<std::string>> keys;
    try {
        return Json::parse(text, [&](int, Json::parse_event_t event, Json& value) {
            if (event == Json::parse_event_t::object_start) keys.emplace_back();
            else if (event == Json::parse_event_t::key && !keys.back().insert(value.get<std::string>()).second)
                invalid("Duplicate JSON object key");
            else if (event == Json::parse_event_t::object_end) keys.pop_back();
            return true;
        });
    } catch (const std::invalid_argument&) { throw; }
    catch (const std::exception& e) { invalid(std::string("Invalid JSON: ") + e.what()); }
}
void fields(const Json& j, std::initializer_list<const char*> names) {
    if (!j.is_object() || j.size() != names.size()) invalid("Unexpected settings fields");
    for (auto name : names) if (!j.contains(name)) invalid(std::string("Missing field: ") + name);
}
std::string text(const Json& j, const char* name) {
    const auto& v = j.at(name);
    if (!v.is_string() || !named(v.get<std::string>())) invalid(std::string("Expected nonblank text: ") + name);
    return v.get<std::string>();
}
std::uint64_t decimal(const Json& j, const char* name) {
    const auto& v = j.at(name);
    if (!v.is_string()) invalid(std::string("Expected decimal string: ") + name);
    const auto s = v.get<std::string>();
    if (s.empty() || s.size() > 20 || !std::all_of(s.begin(), s.end(), [](char c) { return c >= '0' && c <= '9'; }))
        invalid(std::string("Expected unsigned decimal: ") + name);
    std::uint64_t n = 0;
    for (char c : s) {
        const auto digit = static_cast<std::uint64_t>(c - '0');
        if (n > (std::numeric_limits<std::uint64_t>::max() - digit) / 10) invalid(std::string("Exceeds uint64: ") + name);
        n = n * 10 + digit;
    }
    return n;
}
bool starts(const std::string& s, const char* prefix) { return s.rfind(prefix, 0) == 0; }
bool digits(const std::string& s) {
    return !s.empty() && s.size() <= 5 && std::all_of(s.begin(), s.end(), [](char c) { return c >= '0' && c <= '9'; });
}
// The comparison command sends requests over HTTPS only.
void check_endpoint(const std::string& url) {
    if (std::any_of(url.begin(), url.end(), [](unsigned char c) { return c <= 32 || c == '?' || c == '#' || c == '@'; }))
        invalid("Endpoint must not contain spaces, credentials, query or fragment");
    if (!starts(url, "https://")) invalid("Endpoint must use https");
    const auto rest = url.substr(8);
    const auto authority = rest.substr(0, rest.find('/'));
    const auto host_end = starts(authority, "[") ? authority.find(']') + 1 : authority.find(':');
    const auto host = authority.substr(0, host_end);
    const auto port = host_end < authority.size() ? authority.substr(host_end) : std::string();
    if (host.empty() || host == "[" || (!port.empty() && (port[0] != ':' || !digits(port.substr(1)))))
        invalid("Endpoint requires a host and optional numeric port");
}

Json model_json(const ModelIdentity& m) { return {{"provider", m.provider}, {"id", m.id}, {"version", m.version}}; }
ModelIdentity model_from(const Json& j) {
    fields(j, {"provider", "id", "version"});
    return {text(j, "provider"), text(j, "id"), text(j, "version")};
}
}

std::string settings_to_json(const Settings& s) {
    return Json{{"schema_version", 1}, {"endpoint_url", s.endpoint_url},
        {"reader", model_json(s.reader)}, {"judge", model_json(s.judge)},
        {"max_requests", std::to_string(s.max_requests)}, {"max_tokens", std::to_string(s.max_tokens)},
        {"max_cost_nanos", std::to_string(s.max_cost_nanos)}, {"seed", std::to_string(s.seed)}}.dump();
}
Settings settings_from_json(const std::string& input) {
    const auto j = parse(input);
    fields(j, {"schema_version", "endpoint_url", "reader", "judge", "max_requests", "max_tokens", "max_cost_nanos", "seed"});
    if (j.at("schema_version") != 1) invalid("Unsupported settings schema version");
    Settings s;
    s.endpoint_url = text(j, "endpoint_url");
    check_endpoint(s.endpoint_url);
    s.reader = model_from(j.at("reader")); s.judge = model_from(j.at("judge"));
    s.max_requests = decimal(j, "max_requests"); s.max_tokens = decimal(j, "max_tokens");
    s.max_cost_nanos = decimal(j, "max_cost_nanos"); s.seed = decimal(j, "seed");
    return s;
}
std::optional<Settings> load_settings(const std::filesystem::path& file) {
    std::ifstream in(file, std::ios::binary);
    if (!in) {
        if (!std::filesystem::exists(file)) return std::nullopt;
        throw std::runtime_error("Cannot read settings file: " + file.string());
    }
    std::ostringstream content; content << in.rdbuf();
    return settings_from_json(content.str());
}
void save_settings(const std::filesystem::path& file, const Settings& settings) {
    const auto text = settings_to_json(settings);
    (void)settings_from_json(text);
    auto temporary = file; temporary += ".tmp";
    {
        std::ofstream out(temporary, std::ios::binary | std::ios::trunc);
        out << text;
        if (!out.flush()) throw std::runtime_error("Cannot write settings file: " + temporary.string());
    }
    std::filesystem::rename(temporary, file);
}
}
