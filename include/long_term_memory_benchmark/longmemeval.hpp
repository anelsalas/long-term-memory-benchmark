#pragma once

#include <cstddef>
#include <optional>
#include <string>
#include <string_view>
#include <vector>

namespace ltm {

struct DatasetProvenance {
    std::string source_uri;
    std::string revision;
    std::string expected_sha256;
    std::size_t max_bytes = 256u * 1024u * 1024u;
};

struct CandidateMessage {
    std::string source_id;
    std::string role;
    std::string content;
};

struct CandidateSession {
    std::string source_id;
    std::string date;
    std::vector<CandidateMessage> messages;
};

struct CandidateCase {
    std::string case_id;
    std::string question;
    std::string question_date;
    std::vector<CandidateSession> sessions;
};

struct MessageEvaluation {
    std::string source_id;
    std::string original_session_id;
    std::size_t original_message_number;
    std::optional<bool> has_answer;
    std::string annotations_json;
};

struct SessionEvaluation {
    std::string source_id;
    std::string original_session_id;
};

struct EvaluatorCase {
    std::string question_id;
    std::string question_type;
    bool abstention = false;
    std::string answer_json;
    std::vector<std::string> answer_session_ids;
    std::vector<SessionEvaluation> sessions;
    std::vector<MessageEvaluation> messages;
};

struct LoadedLongMemEvalCase {
    DatasetProvenance provenance;
    std::string actual_sha256;
    CandidateCase candidate;
    EvaluatorCase evaluator;
};

bool longmemeval_date_after(const std::string& left, const std::string& right);

enum class HistoryTimePolicy { require_not_after_question, include_all };

LoadedLongMemEvalCase load_longmemeval_case(std::string_view bytes,
    std::string_view question_id, const DatasetProvenance& provenance,
    HistoryTimePolicy time_policy = HistoryTimePolicy::require_not_after_question);
std::string candidate_input_json(const CandidateCase& candidate);
std::string memory_ingestion_json(const CandidateCase& candidate, std::string_view memory_namespace);

} // namespace ltm
