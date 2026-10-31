#pragma once

#include <string>
#include <vector>
#include "memory.hpp"

namespace dmb {

struct RetrievalCheck {
    std::string case_id;
    std::vector<std::string> expected_sources;
    std::vector<Match> matches;
    double retrieval_ms;
};

std::string offline_report(const std::vector<RetrievalCheck>& checks);

} // namespace dmb
