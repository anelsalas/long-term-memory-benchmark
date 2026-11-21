#pragma once

#include <cstddef>
#include <string>
#include <vector>

namespace ltm {

enum class MemoryKind { profile, event, fact, procedure, task };

// A compact reference to a record in an authoritative archive.
struct CatalogEntry {
    std::string id;
    std::string person;
    MemoryKind kind;
    std::vector<std::string> tags;
};

class MemoryCatalog {
public:
    explicit MemoryCatalog(std::size_t capacity);
    void put(CatalogEntry entry);
    void erase(const std::string& person, const std::string& id);
    std::vector<std::string> find(const std::string& person, MemoryKind kind,
                                  const std::string& tag) const;
private:
    std::size_t capacity_;
    std::vector<CatalogEntry> entries_;
};

} // namespace ltm
