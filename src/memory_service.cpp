#include "long_term_memory_benchmark/memory_service.hpp"
#include "long_term_memory_benchmark/retrieval.hpp"
#include <algorithm>
#include <stdexcept>

namespace ltm::retrieval {

std::string render_memory_context(const CandidateCase& candidate) {
    std::string block;
    for (const auto& session : candidate.sessions)
        for (const auto& message : session.messages)
            block += "[" + session.date + "] " + message.content + "\n";
    return block;
}

SelectedMemories select_memories(const CandidateCase& input, MemoryService& service,
    const std::string& name, std::size_t limit, std::size_t budget_tokens) {
    if (name.empty() || limit == 0 || budget_tokens == 0) throw std::invalid_argument("Memory search limits must be positive");
    const auto ranked = service.search(name, input.question, limit);
    SelectedMemories result;
    result.candidate = input;
    result.candidate.sessions.clear();
    std::size_t bytes = 0;
    std::set<std::string> seen;
    for (const auto& memory : ranked) {
        if (memory.id.empty() || memory.text.empty() || !seen.insert(memory.id).second)
            throw std::invalid_argument("Invalid or repeated memory ID");
        const auto line_bytes = memory.session_date.size() + memory.text.size() + 4;
        if (line_bytes > std::string{}.max_size() - bytes ||
            estimated_tokens(bytes + line_bytes) > budget_tokens) break;
        bytes += line_bytes;
        result.memory_ids.push_back(memory.id);
        if (memory.source_sessions.empty()) result.unknown_provenance = true;
        for (const auto& session : memory.source_sessions) {
            if (session.empty()) result.unknown_provenance = true;
            else result.source_sessions.insert(session);
        }
        result.candidate.sessions.push_back({memory.id, memory.session_date,
            {{memory.id, "memory", memory.text}}});
    }
    std::stable_sort(result.candidate.sessions.begin(), result.candidate.sessions.end(),
        [](const auto& left, const auto& right) {
            if (left.date.empty() || right.date.empty()) return !left.date.empty() && right.date.empty();
            if (left.date == right.date) return false;
            return longmemeval_date_after(right.date, left.date);
        });
    return result;
}

} // namespace ltm::retrieval
