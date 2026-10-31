#include "long_term_memory_benchmark/dashboard_settings.hpp"
#include <cstdint>
#include <chrono>
#include <filesystem>
#include <fstream>
#include <functional>
#include <iostream>
#include <limits>
#include <stdexcept>
#include <string>
#include <utility>
#include <vector>

namespace {
using ltm::dashboard::Settings;
constexpr auto u64_max = std::numeric_limits<std::uint64_t>::max();

void require(bool condition, const std::string& message) {
    if (!condition) throw std::runtime_error(message);
}
Settings fixture() {
    Settings s;
    s.endpoint_url = "https://provider.example/v1";
    s.reader = {"example", "reader-model", "2026-10-01"};
    s.judge = {"example", "judge-model", "2026-09-15"};
    s.max_requests = 20; s.max_tokens = u64_max; s.max_cost_nanos = 2500000000; s.seed = u64_max;
    return s;
}
bool same(const ltm::ModelIdentity& a, const ltm::ModelIdentity& b) {
    return a.provider == b.provider && a.id == b.id && a.version == b.version;
}
void roundtrip() {
    const auto s = ltm::dashboard::settings_from_json(ltm::dashboard::settings_to_json(fixture()));
    const auto expected = fixture();
    require(s.endpoint_url == expected.endpoint_url, "Endpoint survives round trip");
    require(same(s.reader, expected.reader) && same(s.judge, expected.judge), "Models survive round trip");
    require(s.max_requests == 20 && s.max_tokens == u64_max, "Request and token caps survive round trip");
    require(s.max_cost_nanos == 2500000000 && s.seed == u64_max, "Cost cap and seed survive round trip");
}
void rejected(const std::string& text, const std::string& why) {
    try { ltm::dashboard::settings_from_json(text); }
    catch (const std::invalid_argument&) { return; }
    throw std::runtime_error("Must reject: " + why);
}
std::string with(const std::string& from, const std::string& to) {
    auto text = ltm::dashboard::settings_to_json(fixture());
    const auto at = text.find(from);
    if (at == std::string::npos) throw std::runtime_error("Fixture lacks " + from);
    return text.replace(at, from.size(), to);
}
void rejects_malformed() {
    rejected("{", "invalid JSON");
    rejected("[]", "non-object");
    rejected(with("\"seed\"", "\"other\""), "unknown field in place of seed");
    rejected(with("{\"endpoint_url\"", "{\"extra\":\"x\",\"endpoint_url\""), "extra field");
    rejected(with("\"schema_version\":1", "\"schema_version\":2"), "unsupported schema version");
    rejected(with("\"reader-model\"", "\" \""), "blank reader model");
    rejected(with("\"2026-09-15\"", "\"\""), "blank judge version");
    rejected(with("\"reader\":{\"id\"", "\"reader\":{\"id\":\"x\",\"id\""), "duplicate key");
    rejected(with("\"max_requests\":\"20\"", "\"max_requests\":20"), "number instead of decimal string");
}
void rejects_bad_decimals() {
    for (const std::string bad : {"", "-1", "+1", " 1", "1.5", "1e3", "0x10", "18446744073709551616", "99999999999999999999"})
        rejected(with("\"max_requests\":\"20\"", "\"max_requests\":\"" + bad + "\""), "decimal '" + bad + "'");
}
std::string endpoint(const std::string& url) { return with("\"https://provider.example/v1\"", "\"" + url + "\""); }
void endpoint_rules() {
    for (const std::string ok : {"https://provider.example/v1", "https://provider.example:8443",
            "http://localhost:8080/v1", "http://127.0.0.1:1234/v1", "http://[::1]:9000/v1"})
        require(ltm::dashboard::settings_from_json(endpoint(ok)).endpoint_url == ok, "Accept endpoint " + ok);
    for (const std::string bad : {"provider.example/v1", "ftp://provider.example", "https://", "https:///v1",
            "http://provider.example/v1", "http://192.168.1.20:8080/v1", "http://localhost.evil.example/v1",
            "https://user:secret@provider.example/v1", "https://provider.example/v1?key=secret",
            "https://provider.example/v1#frag"})
        rejected(endpoint(bad), "endpoint " + bad);
}
struct TempDir {
    std::filesystem::path path = std::filesystem::temp_directory_path() / ("ltm-dashboard-" + std::to_string(
        std::chrono::steady_clock::now().time_since_epoch().count()));
    TempDir() { std::filesystem::create_directories(path); }
    ~TempDir() { std::error_code ignored; std::filesystem::remove_all(path, ignored); }
};
void file_store() {
    const TempDir dir; const auto file = dir.path / "settings.json";
    require(!ltm::dashboard::load_settings(file), "Missing file means no saved settings");
    ltm::dashboard::save_settings(file, fixture());
    const auto loaded = ltm::dashboard::load_settings(file);
    require(loaded && loaded->max_tokens == u64_max && same(loaded->judge, fixture().judge), "Saved settings load back");
    auto changed = fixture(); changed.max_requests = 3;
    ltm::dashboard::save_settings(file, changed);
    require(ltm::dashboard::load_settings(file)->max_requests == 3, "Saving replaces earlier settings");
    require(std::distance(std::filesystem::directory_iterator(dir.path), {}) == 1, "No temporary files remain");
}
void file_store_rejects() {
    const TempDir dir; const auto file = dir.path / "settings.json";
    { std::ofstream(file) << "{\"schema_version\":1}"; }
    bool rejected_file = false;
    try { ltm::dashboard::load_settings(file); } catch (const std::invalid_argument&) { rejected_file = true; }
    require(rejected_file, "A corrupt settings file is reported, not ignored");
    auto bad = fixture(); bad.endpoint_url = "http://192.168.1.20/v1";
    bool rejected_save = false;
    try { ltm::dashboard::save_settings(file, bad); } catch (const std::invalid_argument&) { rejected_save = true; }
    require(rejected_save, "Invalid settings are never written");
}
}

int main(int argc, char** argv) {
    const std::vector<std::pair<std::string, std::function<void()>>> cases = {
        {"roundtrip", roundtrip}, {"rejects_malformed", rejects_malformed},
        {"rejects_bad_decimals", rejects_bad_decimals}, {"endpoint_rules", endpoint_rules},
        {"file_store", file_store}, {"file_store_rejects", file_store_rejects}};
    if (argc != 2) return 2;
    for (const auto& item : cases) {
        if (item.first == argv[1]) {
            try { item.second(); std::cout << "PASS " << item.first << '\n'; return 0; }
            catch (const std::exception& e) { std::cerr << "FAIL " << item.first << ": " << e.what() << '\n'; return 1; }
        }
    }
    return 2;
}
