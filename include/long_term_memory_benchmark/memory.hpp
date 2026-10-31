#pragma once

#include <cstddef>
#include <string>
#include <vector>

namespace ltm {

struct Record {
    std::string id;
    std::string person;
    std::string timestamp; // UTC ISO 8601: YYYY-MM-DDTHH:MM:SSZ
    std::string text;
};

struct Query {
    std::string text;
    std::string person;
    std::size_t limit = 5;
    std::string as_of; // Empty includes the complete supplied history.
};

struct Match {
    Record record;
    std::size_t score;
};

class MemoryPort {
public:
    virtual ~MemoryPort() = default;
    virtual void store(Record record) = 0;
    virtual std::vector<Match> search(const Query& query) const = 0;
};

class LexicalMemory final : public MemoryPort {
public:
    void store(Record record) override;
    std::vector<Match> search(const Query& query) const override;
private:
    std::vector<Record> records_;
};

} // namespace ltm
