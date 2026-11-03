#pragma once

#include "longmemeval.hpp"
#include <cstddef>
#include <set>
#include <string>
#include <vector>

namespace ltm::retrieval {

struct MemoryRecord {
    std::string id;
    std::string text;
    std::string session_date;
    std::vector<std::string> source_sessions;
};

class MemoryService {
public:
    virtual ~MemoryService() = default;
    virtual std::vector<MemoryRecord> search(const std::string& name, const std::string& question,
                                              std::size_t limit) = 0;
};

struct SelectedMemories {
    CandidateCase candidate;
    std::vector<std::string> memory_ids;  // ranking order; these are not original message IDs
    std::set<std::string> source_sessions;
    bool unknown_provenance = false;
};

SelectedMemories select_memories(const CandidateCase& input, MemoryService& service,
    const std::string& name, std::size_t limit, std::size_t budget_tokens);

} // namespace ltm::retrieval
