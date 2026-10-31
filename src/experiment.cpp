#include "long_term_memory_benchmark/experiment.hpp"
#include "long_term_memory_benchmark/hash.hpp"
#include <nlohmann/json.hpp>
#include <algorithm>
#include <limits>
#include <set>
#include <stdexcept>
#include <string_view>

namespace ltm {
namespace {
using Json=nlohmann::json;
[[noreturn]] void invalid(const std::string& message) { throw std::invalid_argument(message); }
std::string dump(const Json& value) {
    try { return value.dump(); }
    catch(const Json::exception& e) { invalid(std::string("Invalid JSON value: ")+e.what()); }
}
bool named(const std::string& s) { return std::any_of(s.begin(),s.end(),[](unsigned char c){return c>32;}); }
bool digest(const std::string& s) {
    return s.size()==64 && std::all_of(s.begin(),s.end(),[](char c){return (c>='0'&&c<='9')||(c>='a'&&c<='f');});
}
Json parse(const std::string& text) {
    std::vector<std::set<std::string>> keys;
    try {
        return Json::parse(text,[&](int, Json::parse_event_t event, Json& value) {
            if(event==Json::parse_event_t::object_start) keys.emplace_back();
            else if(event==Json::parse_event_t::key && !keys.back().insert(value.get<std::string>()).second)
                invalid("Duplicate JSON object key");
            else if(event==Json::parse_event_t::object_end) keys.pop_back();
            return true;
        });
    } catch(const std::exception& e) { invalid(std::string("Invalid JSON: ")+e.what()); }
}
void fields(const Json& j,std::initializer_list<const char*> names) {
    if(!j.is_object() || j.size()!=names.size()) invalid("Unexpected object fields");
    for(auto name:names) if(!j.contains(name)) invalid(std::string("Missing field: ")+name);
}
std::string string(const Json& j) {
    if(!j.is_string()) invalid("Expected string"); return j.get<std::string>();
}
std::uint64_t number(const Json& j) {
    if(j.is_number_unsigned()) return j.get<std::uint64_t>();
    if(j.is_number_integer() && j.get<std::int64_t>()>=0) return static_cast<std::uint64_t>(j.get<std::int64_t>());
    invalid("Expected nonnegative integer");
}
std::uint32_t small_number(const Json& j) {
    const auto n=number(j); if(n>std::numeric_limits<std::uint32_t>::max()) invalid("Integer exceeds uint32");
    return static_cast<std::uint32_t>(n);
}
std::map<std::string,std::string> parameters(const Json& j) {
    if(!j.is_object()) invalid("Expected string parameter object");
    std::map<std::string,std::string> result;
    for(auto it=j.begin();it!=j.end();++it) {
        if(!named(it.key())) invalid("Empty parameter key");
        result.emplace(it.key(),string(it.value()));
    }
    return result;
}
void validate_model(const ModelIdentity& model) {
    if(!named(model.provider)||!named(model.id)||!named(model.version)) invalid("Model identity requires provider/id/version");
}
void validate(const ExperimentManifest& m) {
    if(m.schema_version!=1) invalid("Unsupported manifest version");
    if(!named(m.source_revision)||!named(m.dataset.name)||!named(m.dataset.revision)||!digest(m.dataset.sha256))
        invalid("Source and dataset identity required");
    if(m.case_ids.empty()) invalid("Select at least one case");
    std::set<std::string> ids;
    for(const auto& id:m.case_ids) if(!named(id)||!ids.insert(id).second) invalid("Case IDs must be named and unique");
    if(!named(m.method)||!named(m.method_revision)) invalid("Method identity required");
    validate_model(m.reader); validate_model(m.judge);
    if(m.prompt_sha256.empty()) invalid("Prompt identities required");
    for(const auto& item:m.prompt_sha256) if(!named(item.first)||!digest(item.second)) invalid("Invalid prompt identity");
    for(const auto* p:{&m.method_parameters,&m.run_parameters})
        for(const auto& item:*p) if(!named(item.first)) invalid("Empty parameter key");
    if(!m.context_token_limit||!m.output_token_limit||!m.concurrency) invalid("Token limits and concurrency must be positive");
}
Json model_json(const ModelIdentity& m) {return Json{{"provider",m.provider},{"id",m.id},{"version",m.version}};}
ModelIdentity model_from(const Json& j) {
    fields(j,{"provider","id","version"}); return {string(j.at("provider")),string(j.at("id")),string(j.at("version"))};
}
Json manifest_json(const ExperimentManifest& m) {
    validate(m);
    return Json{{"schema_version",m.schema_version},{"source_revision",m.source_revision},
        {"dataset",{{"name",m.dataset.name},{"revision",m.dataset.revision},{"sha256",m.dataset.sha256}}},
        {"case_ids",m.case_ids},{"method",m.method},{"method_revision",m.method_revision},
        {"method_parameters",m.method_parameters},{"reader",model_json(m.reader)},{"judge",model_json(m.judge)},
        {"prompt_sha256",m.prompt_sha256},{"context_token_limit",m.context_token_limit},
        {"output_token_limit",m.output_token_limit},{"concurrency",m.concurrency},{"run_parameters",m.run_parameters}};
}
std::string status_name(CaseStatus s) {
    switch(s) {
        case CaseStatus::queued:return "queued"; case CaseStatus::running:return "running";
        case CaseStatus::completed:return "completed"; case CaseStatus::failed:return "failed";
        case CaseStatus::uncertain:return "uncertain";
    } invalid("Unknown case status");
}
CaseStatus status_from(const Json& j) {
    const auto value=string(j);
    for(auto status:{CaseStatus::queued,CaseStatus::running,CaseStatus::completed,CaseStatus::failed,CaseStatus::uncertain})
        if(value==status_name(status)) return status;
    invalid("Unknown case status");
}
Json summary_json(const CaseSummary& s) {
    Json j={{"expected",s.expected},{"present",s.present},{"missing",s.missing},{"queued",s.queued},
        {"running",s.running},{"completed",s.completed},{"failed",s.failed},{"uncertain",s.uncertain},
        {"graded",s.graded},{"correct",s.correct},{"completed_ungraded",s.completed_ungraded},
        {"known_correct_fraction_of_expected",s.known_correct_fraction_of_expected},{"graded_accuracy",nullptr}};
    if(s.graded_accuracy) j["graded_accuracy"]=*s.graded_accuracy;
    return j;
}
}

std::string manifest_to_json(const ExperimentManifest& m) { return dump(manifest_json(m)); }
ExperimentManifest manifest_from_json(const std::string& text) {
    const auto j=parse(text);
    fields(j,{"schema_version","source_revision","dataset","case_ids","method","method_revision","method_parameters",
        "reader","judge","prompt_sha256","context_token_limit","output_token_limit","concurrency","run_parameters"});
    ExperimentManifest m; m.schema_version=small_number(j.at("schema_version"));
    m.source_revision=string(j.at("source_revision"));
    const auto& d=j.at("dataset"); fields(d,{"name","revision","sha256"});
    m.dataset={string(d.at("name")),string(d.at("revision")),string(d.at("sha256"))};
    if(!j.at("case_ids").is_array()) invalid("Expected case ID array");
    for(const auto& id:j.at("case_ids")) m.case_ids.push_back(string(id));
    m.method=string(j.at("method")); m.method_revision=string(j.at("method_revision"));
    m.method_parameters=parameters(j.at("method_parameters"));
    m.reader=model_from(j.at("reader")); m.judge=model_from(j.at("judge"));
    m.prompt_sha256=parameters(j.at("prompt_sha256"));
    m.context_token_limit=number(j.at("context_token_limit")); m.output_token_limit=number(j.at("output_token_limit"));
    m.concurrency=small_number(j.at("concurrency")); m.run_parameters=parameters(j.at("run_parameters"));
    validate(m); return m;
}
std::string configuration_hash(const ExperimentManifest& m) { return sha256(manifest_to_json(m)); }

CaseSummary summarize_cases(const ExperimentManifest& m,const std::vector<CaseRecord>& records) {
    validate(m); CaseSummary s; s.expected=m.case_ids.size();
    const std::set<std::string> expected(m.case_ids.begin(),m.case_ids.end()); std::set<std::string> seen;
    for(const auto& r:records) {
        if(!expected.count(r.case_id)||!seen.insert(r.case_id).second) invalid("Unknown or duplicate result case ID");
        (void)status_name(r.status);
        if(r.correct && r.status!=CaseStatus::completed) invalid("Only completed answers may be graded");
        if(r.status==CaseStatus::completed && !r.error.empty()) invalid("Completed case cannot contain error");
        if(r.status==CaseStatus::failed && !named(r.error)) invalid("Failed case requires error");
        ++s.present;
        switch(r.status) {
            case CaseStatus::queued:++s.queued;break; case CaseStatus::running:++s.running;break;
            case CaseStatus::failed:++s.failed;break; case CaseStatus::uncertain:++s.uncertain;break;
            case CaseStatus::completed:
                ++s.completed;
                if(r.correct) {++s.graded;if(*r.correct) ++s.correct;} else ++s.completed_ungraded;
                break;
        }
    }
    s.missing=s.expected-s.present;
    s.known_correct_fraction_of_expected=static_cast<double>(s.correct)/static_cast<double>(s.expected);
    if(s.graded) s.graded_accuracy=static_cast<double>(s.correct)/static_cast<double>(s.graded);
    return s;
}
std::string results_to_json(const ExperimentManifest& m,const std::vector<CaseRecord>& records) {
    const auto summary=summarize_cases(m,records); Json array=Json::array();
    for(const auto& id:m.case_ids) {
        const auto found=std::find_if(records.begin(),records.end(),[&](const CaseRecord& r){return r.case_id==id;});
        if(found==records.end()) continue;
        Json j={{"case_id",found->case_id},{"status",status_name(found->status)},{"answer",found->answer},
            {"error",found->error},{"request_id",found->request_id},{"correct",nullptr}};
        if(found->correct) j["correct"]=*found->correct;
        array.push_back(std::move(j));
    }
    return dump(Json{{"schema_version",1},{"configuration_hash",configuration_hash(m)},
        {"records",array},{"summary",summary_json(summary)}});
}
std::vector<CaseRecord> results_from_json(const ExperimentManifest& m,const std::string& text) {
    const auto j=parse(text); fields(j,{"schema_version","configuration_hash","records","summary"});
    if(number(j.at("schema_version"))!=1) invalid("Unsupported result schema version");
    if(string(j.at("configuration_hash"))!=configuration_hash(m)) invalid("Result configuration mismatch");
    if(!j.at("records").is_array()) invalid("Expected records array");
    std::vector<CaseRecord> records;
    for(const auto& item:j.at("records")) {
        fields(item,{"case_id","status","answer","error","request_id","correct"});
        CaseRecord r{string(item.at("case_id")),status_from(item.at("status")),string(item.at("answer")),
            string(item.at("error")),string(item.at("request_id")),{}};
        if(!item.at("correct").is_null()) {
            if(!item.at("correct").is_boolean()) invalid("Grade must be boolean or null");
            r.correct=item.at("correct").get<bool>();
        }
        records.push_back(std::move(r));
    }
    if(summary_json(summarize_cases(m,records))!=j.at("summary")) invalid("Result summary does not match case records");
    return records;
}
}
