#include "long_term_memory_benchmark/embeddings.hpp"
#include <nlohmann/json.hpp>
#include <chrono>
#include <filesystem>
#include <functional>
#include <iostream>
#include <stdexcept>
#include <string>
#include <utility>
#include <vector>

namespace {
using namespace ltm::retrieval;
using Json = nlohmann::json;
using Vectors = std::vector<std::vector<float>>;

void require(bool condition, const std::string& message) {
    if (!condition) throw std::runtime_error(message);
}
template<class F> bool rejected(F action) {
    try { action(); } catch (const std::invalid_argument&) { return true; }
    return false;
}

void request_lists_inputs() {
    const auto body = Json::parse(embedding_request("embed-1", {"first \"turn\"", "second"}));
    require(body == Json{{"model", "embed-1"}, {"input", {"first \"turn\"", "second"}}}, "Model and inputs in order");
}
void parse_orders_by_index() {
    const auto vectors = parse_embeddings(R"({"object":"list","data":[
        {"object":"embedding","index":1,"embedding":[0.5,-1]},
        {"object":"embedding","index":0,"embedding":[2,0.25]}],"model":"m"})", 2);
    require(vectors == Vectors{{2, 0.25f}, {0.5f, -1}}, "Vectors follow their index");
}
void parse_rejects_bad_responses() {
    const auto item = [](int index, const std::string& embedding) {
        return R"({"index":)" + std::to_string(index) + R"(,"embedding":)" + embedding + "}";
    };
    require(rejected([] { parse_embeddings("not json", 1); }), "Invalid JSON");
    require(rejected([] { parse_embeddings(R"({"error":{"message":"too long"}})", 1); }), "Error body");
    require(rejected([&] { parse_embeddings(R"({"data":[)" + item(0, "[1]") + "]}", 2); }), "Too few vectors");
    require(rejected([&] { parse_embeddings(R"({"data":[)" + item(0, "[1]") + "," + item(0, "[2]") + "]}", 2); }), "Repeated index");
    require(rejected([&] { parse_embeddings(R"({"data":[)" + item(0, "[1,2]") + "," + item(1, "[2]") + "]}", 2); }), "Mixed dimensions");
    require(rejected([&] { parse_embeddings(R"({"data":[)" + item(0, "[]") + "]}", 1); }), "Empty vector");
    require(rejected([&] { parse_embeddings(R"({"data":[)" + item(0, R"(["x"])") + "]}", 1); }), "Non-numeric value");
}
struct FakeEmbedder : Embedder {
    std::vector<std::vector<std::string>> requests;
    Vectors embed(const std::vector<std::string>& texts) override {
        requests.push_back(texts);
        Vectors vectors;
        for (const auto& text : texts) vectors.push_back({static_cast<float>(text.size()), 0.5f});
        return vectors;
    }
};
struct TempDir {
    std::filesystem::path path = std::filesystem::temp_directory_path() / ("ltm-embed-" + std::to_string(
        std::chrono::steady_clock::now().time_since_epoch().count()));
    ~TempDir() { std::error_code ignored; std::filesystem::remove_all(path, ignored); }
};
using Texts = std::vector<std::string>;
void cache_asks_only_for_new_texts() {
    const TempDir dir; FakeEmbedder inner;
    CachedEmbedder cache(inner, dir.path);
    require(cache.embed({"ab", "abcd", "ab"}) == Vectors{{2, 0.5f}, {4, 0.5f}, {2, 0.5f}}, "Vectors in input order");
    require(inner.requests == std::vector<Texts>{{"ab", "abcd"}}, "Each distinct text is asked once");
    require(cache.embed({"abcd", "xyz"}) == Vectors{{4, 0.5f}, {3, 0.5f}}, "Cached and new vectors combine");
    require(inner.requests.back() == Texts{"xyz"}, "Only the new text is asked");
    FakeEmbedder fresh;
    CachedEmbedder reopened(fresh, dir.path);
    require(reopened.embed({"ab", "xyz"}) == Vectors{{2, 0.5f}, {3, 0.5f}} && fresh.requests.empty(), "The cache survives restarts");
    require(cache.embed({}).empty() && inner.requests.size() == 2, "No texts, no request");
}
void cache_only_refuses_unknown_texts() {
    const TempDir dir; FakeEmbedder warm;
    CachedEmbedder(warm, dir.path).embed({"known"});
    CacheOnlyEmbedder none;
    CachedEmbedder cache(none, dir.path);
    require(cache.embed({"known"}) == Vectors{{5, 0.5f}}, "Cached texts are served without an embedder");
    bool refused = false;
    try { cache.embed({"known", "new"}); } catch (const std::runtime_error&) { refused = true; }
    require(refused, "A text missing from the cache is an error");
}
}

int main(int argc, char** argv) {
    const std::vector<std::pair<std::string, std::function<void()>>> cases = {
        {"request_lists_inputs", request_lists_inputs}, {"parse_orders_by_index", parse_orders_by_index},
        {"parse_rejects_bad_responses", parse_rejects_bad_responses},
        {"cache_asks_only_for_new_texts", cache_asks_only_for_new_texts},
        {"cache_only_refuses_unknown_texts", cache_only_refuses_unknown_texts}};
    if (argc != 2) return 2;
    for (const auto& item : cases) {
        if (item.first == argv[1]) {
            try { item.second(); std::cout << "PASS " << item.first << '\n'; return 0; }
            catch (const std::exception& e) { std::cerr << "FAIL " << item.first << ": " << e.what() << '\n'; return 1; }
        }
    }
    return 2;
}
