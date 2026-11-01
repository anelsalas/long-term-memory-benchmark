// Adapted from LongMemEval 9e0b455f4ef0e2ab8f2e582289761153549043fc:
// src/retrieval/eval_utils.py and src/evaluation/evaluate_qa.py.
// MIT License
// 
// Copyright (c) 2024 Di Wu
// 
// Permission is hereby granted, free of charge, to any person obtaining a copy
// of this software and associated documentation files (the "Software"), to deal
// in the Software without restriction, including without limitation the rights
// to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
// copies of the Software, and to permit persons to whom the Software is
// furnished to do so, subject to the following conditions:
// 
// The above copyright notice and this permission notice shall be included in all
// copies or substantial portions of the Software.
// 
// THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
// IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
// FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
// AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
// LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
// OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
// SOFTWARE.
// 
#include "long_term_memory_benchmark/scoring.hpp"
#include "long_term_memory_benchmark/hash.hpp"
#include <nlohmann/json.hpp>
#include <algorithm>
#include <cmath>
#include <map>
#include <set>
#include <stdexcept>
#include <utility>

namespace ltm {
namespace {
struct Document { std::string opaque, original; };
bool is_abs(const EvaluatorCase& e) { return e.question_id.find("_abs") != std::string::npos; }
std::string noans(std::string id) {
    std::size_t p=0;
    while ((p=id.find("answer",p))!=std::string::npos) { id.replace(p,6,"noans"); p+=5; }
    return id;
}
std::string session_id(const std::string& id) {
    auto p=id.rfind('_'); return p==std::string::npos ? std::string{} : id.substr(0,p);
}
double dcg(const std::vector<int>& r,std::size_t k) {
    if(r.empty()||k==0) return 0;
    double v=r[0];
    for(std::size_t i=1;i<std::min(k,r.size());++i) v+=r[i]/std::log2(static_cast<double>(i+1));
    return v;
}
const std::string patterns[]={
R"LTM(I will give you a question, a correct answer, and a response from a model. Please answer yes if the response contains the correct answer. Otherwise, answer no. If the response is equivalent to the correct answer or contains all the intermediate steps to get the correct answer, you should also answer yes. If the response only contains a subset of the information required by the answer, answer no. 

Question: {}

Correct Answer: {}

Model Response: {}

Is the model response correct? Answer yes or no only.)LTM",
R"LTM(I will give you a question, a correct answer, and a response from a model. Please answer yes if the response contains the correct answer. Otherwise, answer no. If the response is equivalent to the correct answer or contains all the intermediate steps to get the correct answer, you should also answer yes. If the response only contains a subset of the information required by the answer, answer no. In addition, do not penalize off-by-one errors for the number of days. If the question asks for the number of days/weeks/months, etc., and the model makes off-by-one errors (e.g., predicting 19 days when the answer is 18), the model's response is still correct. 

Question: {}

Correct Answer: {}

Model Response: {}

Is the model response correct? Answer yes or no only.)LTM",
R"LTM(I will give you a question, a correct answer, and a response from a model. Please answer yes if the response contains the correct answer. Otherwise, answer no. If the response contains some previous information along with an updated answer, the response should be considered as correct as long as the updated answer is the required answer.

Question: {}

Correct Answer: {}

Model Response: {}

Is the model response correct? Answer yes or no only.)LTM",
R"LTM(I will give you a question, a rubric for desired personalized response, and a response from a model. Please answer yes if the response satisfies the desired response. Otherwise, answer no. The model does not need to reflect all the points in the rubric. The response is correct as long as it recalls and utilizes the user's personal information correctly.

Question: {}

Rubric: {}

Model Response: {}

Is the model response correct? Answer yes or no only.)LTM",
R"LTM(I will give you an unanswerable question, an explanation, and a response from a model. Please answer yes if the model correctly identifies the question as unanswerable. The model could say that the information is incomplete, or some other information is given but the asked information is not.

Question: {}

Explanation: {}

Model Response: {}

Does the model correctly identify the question as unanswerable? Answer yes or no only.)LTM"
};
std::string format(const std::string& pattern,const std::string& q,const std::string& gold,std::string_view response) {
    const std::string args[]={q,gold,std::string(response)};
    std::string out;std::size_t pos=0;
    for(const auto& a:args) {
        auto next=pattern.find("{}",pos);
        if(next==std::string::npos) throw std::logic_error("Invalid frozen template");
        out.append(pattern,pos,next-pos);out+=a;pos=next+2;
    }
    out.append(pattern,pos,std::string::npos);return out;
}
}

OfficialRetrievalScore score_official_retrieval(const LoadedLongMemEvalCase& data,
    const std::vector<std::string>& ranking,std::size_t k,RetrievalLevel level) {
    if(k==0) throw std::invalid_argument("Positive k required");
    if(level!=RetrievalLevel::session&&level!=RetrievalLevel::turn&&level!=RetrievalLevel::turn_to_session)
        throw std::invalid_argument("Unknown retrieval level");
    OfficialRetrievalScore result{data.evaluator.question_id,RetrievalExclusion::none,std::nullopt};
    if(is_abs(data.evaluator)) { result.exclusion=RetrievalExclusion::abstention;return result; }
    std::map<std::string,SessionEvaluation> sessions;
    std::map<std::string,MessageEvaluation> messages;
    for(const auto& s:data.evaluator.sessions)
        if(!sessions.emplace(s.source_id,s).second) throw std::invalid_argument("Duplicate session mapping");
    for(const auto& m:data.evaluator.messages)
        if(!messages.emplace(m.source_id,m).second) throw std::invalid_argument("Duplicate message mapping");
    if(sessions.size()!=data.candidate.sessions.size()) throw std::invalid_argument("Session sidecar size mismatch");
    std::vector<Document> corpus;bool user_target=false;
    std::size_t message_count=0;
    for(const auto& s:data.candidate.sessions) {
        auto si=sessions.find(s.source_id);
        if(si==sessions.end()) throw std::invalid_argument("Missing session mapping");
        const auto& original=si->second.original_session_id;
        if(original.empty()) throw std::invalid_argument("Invalid original session identity");
        bool session_target=false;
        std::size_t message_number=0;
        for(const auto& m:s.messages) {
            ++message_number;++message_count;
            auto mi=messages.find(m.source_id);
            if(mi==messages.end()||mi->second.original_session_id!=original||mi->second.original_message_number!=message_number)
                throw std::invalid_argument("Missing or mismatched message mapping");
            if(m.role!="user"&&m.role!="assistant") throw std::invalid_argument("Unsupported candidate message role");
            if(m.role!="user") continue;
            auto flag=mi->second.has_answer;
            bool marked=original.find("answer")!=std::string::npos;
            if(marked&&!flag) throw std::invalid_argument("Answer-marked user requires Boolean annotation");
            session_target=session_target||flag.value_or(false);user_target=user_target||flag.value_or(false);
            if(level!=RetrievalLevel::session) {
                auto id=original+"_"+std::to_string(mi->second.original_message_number);
                if(marked&&!*flag) id=noans(id);
                corpus.push_back({m.source_id,id});
            }
        }
        if(level==RetrievalLevel::session) {
            auto id=original;
            if(id.find("answer")!=std::string::npos&&!session_target) id=noans(id);
            corpus.push_back({s.source_id,id});
        }
    }
    if(message_count!=messages.size()) throw std::invalid_argument("Message sidecar size mismatch");
    if(!user_target) { result.exclusion=RetrievalExclusion::no_user_answer_label;return result; }
    if(ranking.size()!=corpus.size()) throw std::invalid_argument("Complete corpus ranking required");
    std::map<std::string,std::string> map;
    std::set<std::string> correct,seen;
    std::vector<std::string> corpus_ids,ranked;
    for(const auto& d:corpus) {
        if(!map.emplace(d.opaque,d.original).second) throw std::invalid_argument("Duplicate corpus identity");
        corpus_ids.push_back(d.original);
        if(d.original.find("answer")!=std::string::npos) correct.insert(d.original);
    }
    for(const auto& id:ranking) {
        auto it=map.find(id);
        if(it==map.end()||!seen.insert(id).second) throw std::invalid_argument("Unknown or duplicate ranked identity");
        ranked.push_back(it->second);
    }
    std::size_t effective=k;
    if(level==RetrievalLevel::turn_to_session) {
        std::set<std::string> mapped;
        for(const auto& id:correct) mapped.insert(session_id(id));
        correct=std::move(mapped);
        for(auto& id:corpus_ids) id=session_id(id);
        for(auto& id:ranked) id=session_id(id);
        while(effective<=corpus_ids.size()) {
            std::set<std::string> unique;
            for(std::size_t i=0;i<std::min(effective,ranked.size());++i) unique.insert(ranked[i]);
            if(unique.size()>=k) break;
            ++effective;
        }
    }
    std::set<std::string> recalled;
    std::vector<int> relevant,ideal;
    for(std::size_t i=0;i<ranked.size();++i) {
        relevant.push_back(correct.count(ranked[i])?1:0);
        if(i<effective) recalled.insert(ranked[i]);
    }
    for(const auto& id:corpus_ids) ideal.push_back(correct.count(id)?1:0);
    std::sort(ideal.begin(),ideal.end(),std::greater<int>());
    auto any=std::any_of(correct.begin(),correct.end(),[&](const auto& id){return recalled.count(id)!=0;});
    auto all=std::all_of(correct.begin(),correct.end(),[&](const auto& id){return recalled.count(id)!=0;});
    auto denominator=dcg(ideal,effective);
    result.metrics=OfficialRetrievalMetrics{any?1.0:0.0,all?1.0:0.0,
        denominator==0?0:dcg(relevant,effective)/denominator,effective};
    return result;
}

RetrievalAggregate aggregate_official_retrieval(const std::vector<std::string>& ids,
    const std::vector<OfficialRetrievalScore>& scores) {
    RetrievalAggregate a;std::set<std::string> expected(ids.begin(),ids.end()),seen;
    if(expected.size()!=ids.size()) throw std::invalid_argument("Duplicate expected question");
    a.expected=expected.size();double any=0,all=0,ndcg=0;
    for(const auto& s:scores) {
        if(!expected.count(s.question_id)||!seen.insert(s.question_id).second)
            throw std::invalid_argument("Unknown or duplicate scored question");
        ++a.supplied;
        if(s.exclusion==RetrievalExclusion::abstention) {
            if(s.metrics) throw std::invalid_argument("Excluded metrics present");
            ++a.abstention;
        } else if(s.exclusion==RetrievalExclusion::no_user_answer_label) {
            if(s.metrics) throw std::invalid_argument("Excluded metrics present");
            ++a.no_user_answer_label;
        } else {
            if(s.exclusion!=RetrievalExclusion::none||!s.metrics) throw std::invalid_argument("Missing eligible metrics");
            const auto& m=*s.metrics;
            if((m.recall_any!=0&&m.recall_any!=1)||(m.recall_all!=0&&m.recall_all!=1)||
                !std::isfinite(m.ndcg)||m.ndcg<0||m.ndcg>1||m.effective_k==0)
                throw std::invalid_argument("Invalid official metric");
            ++a.eligible;any+=m.recall_any;all+=m.recall_all;ndcg+=m.ndcg;
        }
    }
    a.missing=a.expected-a.supplied;
    if(a.eligible) {a.mean_recall_any=any/a.eligible;a.mean_recall_all=all/a.eligible;a.mean_ndcg=ndcg/a.eligible;}
    return a;
}

OfficialGradeRequest make_official_grade_request(const LoadedLongMemEvalCase& data,
    std::string_view response,std::string model) {
    if(model.empty()) throw std::invalid_argument("Judge model identity required");
    nlohmann::json gold;
    try {gold=nlohmann::json::parse(data.evaluator.answer_json);}
    catch(const nlohmann::json::exception&) {throw std::invalid_argument("Invalid evaluator gold JSON");}
    if(!gold.is_string()) throw std::invalid_argument("Exact official prompt profile requires string gold");
    std::size_t index=0;const auto& type=data.evaluator.question_type;
    if(is_abs(data.evaluator)) index=4;
    else if(type=="single-session-user"||type=="single-session-assistant"||type=="multi-session") index=0;
    else if(type=="temporal-reasoning") index=1;
    else if(type=="knowledge-update") index=2;
    else if(type=="single-session-preference") index=3;
    else throw std::invalid_argument("Unknown official QA task");
    auto prompt=format(patterns[index],data.candidate.question,gold.get<std::string>(),response);
    return {std::move(model),prompt,sha256(prompt),sha256(patterns[index]),0,10};
}
JudgeVerdict parse_judge_verdict(std::string_view value) {
    auto space=[](char c){return c==' '||c=='\n'||c=='\r'||c=='\t'||c=='\v'||c=='\f';};
    while(!value.empty()&&space(value.front())) value.remove_prefix(1);
    while(!value.empty()&&space(value.back())) value.remove_suffix(1);
    std::string result(value);
    for(auto& c:result) if(c>='A'&&c<='Z') c=static_cast<char>(c+('a'-'A'));
    if(result=="yes") return JudgeVerdict::yes;
    if(result=="no") return JudgeVerdict::no;
    return JudgeVerdict::invalid;
}
} // namespace ltm
