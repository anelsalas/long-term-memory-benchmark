#pragma once

#include "longmemeval.hpp"
#include <cstddef>
#include <optional>
#include <string>
#include <string_view>
#include <vector>

namespace ltm {
enum class RetrievalLevel { session, turn, turn_to_session };
enum class RetrievalExclusion { none, abstention, no_user_answer_label };
struct OfficialRetrievalMetrics {
    double recall_any = 0;
    double recall_all = 0;
    double ndcg = 0;
    std::size_t effective_k = 0;
};
struct OfficialRetrievalScore {
    std::string question_id;
    RetrievalExclusion exclusion = RetrievalExclusion::none;
    std::optional<OfficialRetrievalMetrics> metrics;
};
OfficialRetrievalScore score_official_retrieval(const LoadedLongMemEvalCase& data,
    const std::vector<std::string>& ranked_opaque_sources, std::size_t k,
    RetrievalLevel level);

struct RetrievalAggregate {
    std::size_t expected = 0;
    std::size_t supplied = 0;
    std::size_t eligible = 0;
    std::size_t abstention = 0;
    std::size_t no_user_answer_label = 0;
    std::size_t missing = 0;
    std::optional<double> mean_recall_any;
    std::optional<double> mean_recall_all;
    std::optional<double> mean_ndcg;
};
RetrievalAggregate aggregate_official_retrieval(const std::vector<std::string>& expected_ids,
    const std::vector<OfficialRetrievalScore>& scores);

struct OfficialGradeRequest {
    std::string model;
    std::string prompt;
    std::string prompt_sha256;
    std::string template_sha256;
    double temperature = 0;
    std::size_t max_output_tokens = 10;
    std::string message_role = "user";
    std::size_t sample_count = 1;
};
OfficialGradeRequest make_official_grade_request(const LoadedLongMemEvalCase& data,
    std::string_view frozen_candidate_response,
    std::string model = "gpt-4o-2024-08-06");
enum class JudgeVerdict { yes, no, invalid };
JudgeVerdict parse_judge_verdict(std::string_view response);
} // namespace ltm
