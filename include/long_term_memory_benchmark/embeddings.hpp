#pragma once

#include "retrieval.hpp"
#include <cstddef>
#include <filesystem>
#include <string>
#include <string_view>
#include <vector>

namespace ltm::retrieval {

// OpenAI-compatible /v1/embeddings request body.
std::string embedding_request(const std::string& model, const std::vector<std::string>& texts);

// Vectors from an /v1/embeddings response, ordered by each item's "index".
// Rejects a response that does not hold exactly `expected` vectors of one dimension.
std::vector<std::vector<float>> parse_embeddings(std::string_view body, std::size_t expected);

// Keeps each text's vector in directory/<sha256 of text>.f32 and asks the inner
// embedder only for texts it has not stored, once per distinct text.
class CachedEmbedder final : public Embedder {
public:
    CachedEmbedder(Embedder& inner, std::filesystem::path directory);
    std::vector<std::vector<float>> embed(const std::vector<std::string>& texts) override;
private:
    Embedder& inner_;
    std::filesystem::path directory_;
};

} // namespace ltm::retrieval
