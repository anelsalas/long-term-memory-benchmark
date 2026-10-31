#pragma once

#include <string>
#include <vector>
#include "memory.hpp"
#include "resources.hpp"

namespace ltm {

struct RetrievalCheck {
    std::string case_id;
    std::vector<std::string> expected_sources;
    std::vector<Match> matches;
    double retrieval_ms;
};

std::string offline_report(const std::vector<RetrievalCheck>& checks,
    const std::vector<ResourceMeasurement>& resources = {});

} // namespace ltm
