#include "long_term_memory_benchmark/memory_catalog.hpp"

#include <algorithm>
#include <stdexcept>
#include <utility>

namespace ltm {

MemoryCatalog::MemoryCatalog(std::size_t capacity) : capacity_(capacity) {
    if (!capacity) throw std::invalid_argument("Catalog capacity must be positive");
}

void MemoryCatalog::put(CatalogEntry entry) {
    if (entry.id.empty() || entry.person.empty() || entry.tags.empty() ||
        std::any_of(entry.tags.begin(), entry.tags.end(), [](const auto& tag) { return tag.empty(); }))
        throw std::invalid_argument("Catalog entry needs an ID, person and tags");
    const auto existing = std::find_if(entries_.begin(), entries_.end(),
        [&](const auto& item) { return item.id == entry.id; });
    if (existing != entries_.end()) {
        if (existing->person != entry.person) throw std::invalid_argument("Catalog owner conflict");
        *existing = std::move(entry);
        return;
    }
    if (entries_.size() >= capacity_) throw std::length_error("Catalog capacity reached");
    entries_.push_back(std::move(entry));
}

void MemoryCatalog::erase(const std::string& person, const std::string& id) {
    if (person.empty() || id.empty()) throw std::invalid_argument("Delete needs a person and ID");
    entries_.erase(std::remove_if(entries_.begin(), entries_.end(),
        [&](const auto& item) { return item.person == person && item.id == id; }), entries_.end());
}

std::vector<std::string> MemoryCatalog::find(const std::string& person, MemoryKind kind,
                                             const std::string& tag) const {
    if (person.empty() || tag.empty()) throw std::invalid_argument("Search needs a person and tag");
    std::vector<std::string> ids;
    for (const auto& entry : entries_)
        if (entry.person == person && entry.kind == kind &&
            std::find(entry.tags.begin(), entry.tags.end(), tag) != entry.tags.end())
            ids.push_back(entry.id);
    std::sort(ids.begin(), ids.end());
    return ids;
}

} // namespace ltm
