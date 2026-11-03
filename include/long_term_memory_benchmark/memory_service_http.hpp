#pragma once

#include "memory_service.hpp"
#include <chrono>
#include <string>

namespace ltm::retrieval {

class HttpMemoryService final : public MemoryService {
public:
    explicit HttpMemoryService(const std::string& base_url,
        std::chrono::milliseconds timeout = std::chrono::minutes(5));
    std::vector<MemoryRecord> search(const std::string& name, const std::string& question,
                                     std::size_t limit) override;
    std::string config_json();
private:
    std::string request(const std::string& target, const std::string& body, bool post);
    std::string host_, port_;
    std::chrono::milliseconds timeout_;
};

} // namespace ltm::retrieval
