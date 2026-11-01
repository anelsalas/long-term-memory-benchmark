#pragma once

#include "retrieval.hpp"
#include <chrono>
#include <cstddef>
#include <string>
#include <vector>

namespace ltm::retrieval {

// Sends batches to an OpenAI-compatible /v1/embeddings endpoint on this machine.
// Only http://127.0.0.1, http://localhost and http://[::1] URLs are accepted.
// A request holds at most batch_texts texts and batch_tokens estimated tokens
// (the server's context is shared by a batch). Longer texts are cut to
// text_bytes at a UTF-8 character boundary.
struct EmbeddingLimits {
    std::size_t batch_texts = 32;
    std::size_t batch_tokens = 6000;
    std::size_t text_bytes = 16000;
};

class HttpEmbedder final : public Embedder {
public:
    HttpEmbedder(const std::string& url, std::string model, EmbeddingLimits limits = {},
        std::chrono::milliseconds timeout = std::chrono::minutes(5));
    std::vector<std::vector<float>> embed(const std::vector<std::string>& texts) override;
    std::size_t truncated() const { return truncated_; }  // texts cut so far
private:
    std::vector<std::vector<float>> post(const std::vector<std::string>& batch);
    std::string host_, port_, target_, model_;
    EmbeddingLimits limits_;
    std::chrono::milliseconds timeout_;
    std::size_t truncated_ = 0;
};

} // namespace ltm::retrieval
