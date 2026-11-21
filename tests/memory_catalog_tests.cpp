#include "long_term_memory_benchmark/memory_catalog.hpp"

#include <iostream>
#include <stdexcept>
#include <string>
#include <vector>

namespace {
void require(bool result, const char* message) {
    if (!result) throw std::runtime_error(message);
}
void test_catalog() {
    using namespace ltm;
    MemoryCatalog catalog(3);
    catalog.put({"meeting", "maya", MemoryKind::event, {"school", "march"}});
    catalog.put({"grade", "maya", MemoryKind::fact, {"school", "grade"}});
    catalog.put({"other-grade", "sam", MemoryKind::fact, {"school", "grade"}});
    require(catalog.find("maya", MemoryKind::event, "school") == std::vector<std::string>{"meeting"},
            "Find Maya's school event");
    require(catalog.find("maya", MemoryKind::fact, "grade") == std::vector<std::string>{"grade"},
            "Keep Maya's and Sam's facts separate");
    require(catalog.find("maya", MemoryKind::task, "school").empty(), "Different memory kinds remain separate");
    bool full = false;
    try { catalog.put({"more", "maya", MemoryKind::task, {"school"}}); }
    catch (const std::length_error&) { full = true; }
    require(full && catalog.find("maya", MemoryKind::event, "school").size() == 1,
            "Capacity failures preserve existing references");
    catalog.put({"grade", "maya", MemoryKind::fact, {"corrected"}});
    require(catalog.find("maya", MemoryKind::fact, "grade").empty() &&
            catalog.find("maya", MemoryKind::fact, "corrected").size() == 1,
            "Updating an entry replaces its old tags");
    bool owner_conflict = false;
    try { catalog.put({"grade", "sam", MemoryKind::fact, {"grade"}}); }
    catch (const std::invalid_argument&) { owner_conflict = true; }
    require(owner_conflict && catalog.find("maya", MemoryKind::fact, "corrected").size() == 1,
            "Another person cannot replace the reference");
    catalog.erase("sam", "grade");
    require(catalog.find("maya", MemoryKind::fact, "corrected").size() == 1,
            "Another person cannot remove the reference");
    catalog.erase("maya", "grade");
    require(catalog.find("maya", MemoryKind::fact, "corrected").empty(), "Deleted references cannot be returned");
}
}

int main() {
    try { test_catalog(); std::cout << "PASS memory_catalog\n"; return 0; }
    catch (const std::exception& error) { std::cerr << "FAIL " << error.what() << '\n'; return 1; }
}
