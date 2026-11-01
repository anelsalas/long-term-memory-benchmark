#include "long_term_memory_benchmark/embeddings.hpp"

#include "long_term_memory_benchmark/hash.hpp"
#include <nlohmann/json.hpp>
#include <algorithm>
#include <fstream>
#include <map>
#include <stdexcept>
#include <utility>

namespace ltm::retrieval {
namespace {
using Json = nlohmann::json;
[[noreturn]] void invalid(const std::string& why) { throw std::invalid_argument("Embedding response: " + why); }
}

std::string embedding_request(const std::string& model, const std::vector<std::string>& texts) {
    return Json{{"model", model}, {"input", texts}}.dump();
}

std::vector<std::vector<float>> parse_embeddings(std::string_view body, std::size_t expected) {
    const auto json = Json::parse(body, nullptr, false);
    if (json.is_discarded() || !json.is_object()) invalid("not a JSON object");
    if (!json.contains("data") || !json["data"].is_array()) invalid("no data array");
    const auto& data = json["data"];
    if (data.size() != expected) invalid("expected " + std::to_string(expected) + " vectors");
    std::vector<std::vector<float>> vectors(expected);
    for (const auto& item : data) {
        if (!item.is_object() || !item.contains("index") || !item["index"].is_number_unsigned()) invalid("bad index");
        const auto index = item["index"].get<std::size_t>();
        if (index >= expected || !vectors[index].empty()) invalid("repeated or out-of-range index");
        if (!item.contains("embedding") || !item["embedding"].is_array() || item["embedding"].empty()) invalid("no vector");
        for (const auto& value : item["embedding"]) {
            if (!value.is_number()) invalid("non-numeric value");
            vectors[index].push_back(value.get<float>());
        }
    }
    for (const auto& vector : vectors)
        if (vector.size() != vectors.front().size()) invalid("mixed dimensions");
    return vectors;
}

CachedEmbedder::CachedEmbedder(Embedder& inner, std::filesystem::path directory)
    : inner_(inner), directory_(std::move(directory)) {}

std::vector<std::vector<float>> CachedEmbedder::embed(const std::vector<std::string>& texts) {
    std::map<std::string, std::vector<float>> known;
    std::vector<std::string> missing;
    for (const auto& text : texts) {
        if (known.count(text)) continue;
        std::ifstream in(directory_ / (sha256(text) + ".f32"), std::ios::binary);
        if (in) {
            auto& vector = known[text];
            float value;
            while (in.read(reinterpret_cast<char*>(&value), sizeof value)) vector.push_back(value);
        } else if (std::find(missing.begin(), missing.end(), text) == missing.end()) {
            missing.push_back(text);
        }
    }
    if (!missing.empty()) {
        const auto vectors = inner_.embed(missing);
        if (vectors.size() != missing.size()) throw std::runtime_error("Embedder returned the wrong number of vectors");
        std::filesystem::create_directories(directory_);
        for (std::size_t i = 0; i < missing.size(); ++i) {
            const auto file = directory_ / (sha256(missing[i]) + ".f32");
            const auto partial = std::filesystem::path(file).concat(".tmp");
            {
                std::ofstream out(partial, std::ios::binary | std::ios::trunc);
                out.write(reinterpret_cast<const char*>(vectors[i].data()),
                    static_cast<std::streamsize>(vectors[i].size() * sizeof(float)));
                if (!out.flush()) throw std::runtime_error("Could not write " + partial.string());
            }
            std::filesystem::rename(partial, file);
            known[missing[i]] = vectors[i];
        }
    }
    std::vector<std::vector<float>> result;
    for (const auto& text : texts) result.push_back(known.at(text));
    return result;
}

} // namespace ltm::retrieval
