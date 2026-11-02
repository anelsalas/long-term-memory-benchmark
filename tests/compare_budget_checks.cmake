cmake_minimum_required(VERSION 3.20)

# Behavior of the bm25_budget condition in ltm-compare, checked with dry runs.
foreach(required IN ITEMS LTM_COMPARE_EXE LTM_TEST_DIR)
  if(NOT DEFINED ${required} OR "${${required}}" STREQUAL "")
    message(FATAL_ERROR "Define ${required} for budget checks")
  endif()
endforeach()
string(RANDOM LENGTH 12 ALPHABET 0123456789abcdef nonce)
set(work "${LTM_TEST_DIR}/compare-budget-${nonce}")
file(MAKE_DIRECTORY "${work}")

function(write_config path conditions budget_field)
  if(NOT DEFINED profile)
    set(profile "compact_local_smoke")
  endif()
  if(NOT DEFINED input_bound)
    set(input_bound "512")
  endif()
  file(WRITE "${path}" "{
  \"schema_version\": 1, \"source_revision\": \"test\",
  \"endpoint_url\": \"http://127.0.0.1:8766/v1/chat/completions\",
  \"execution_mode\": \"local\", \"prompt_profile\": \"${profile}\", \"fresh_distractors\": 6,
  \"conditions\": [${conditions}], ${budget_field}
  \"reader\": {\"id\": \"r\", \"version\": \"r\"}, \"judge\": {\"id\": \"r\", \"version\": \"r\"},
  \"provider_policy\": {\"max_requests\": \"8\", \"max_tokens\": \"4096\", \"max_cost_nanos\": \"0\",
    \"input_token_price_nanos\": \"0\", \"output_token_price_nanos\": \"0\", \"request_price_nanos\": \"0\",
    \"max_prompt_bytes\": \"4000000\", \"max_response_bytes\": \"1000000\", \"max_attempts\": 1},
  \"input_token_upper_bound\": \"${input_bound}\", \"context_token_limit\": \"32768\", \"output_token_limit\": \"500\",
  \"judge_output_token_limit\": \"32\", \"seed\": \"42\", \"top_k\": 1, \"timeout_ms\": 60000,
  \"output_token_parameter\": \"max_tokens\", \"temperature\": null,
  \"output_directory\": \"artifacts\", \"chat_framing_token_allowance\": 32, \"tokenizer_bound_verified\": true
}")
endfunction()

function(dry_run config result_code result_output)
  execute_process(COMMAND "${LTM_COMPARE_EXE}" --config "${config}" --dry-run
    RESULT_VARIABLE code OUTPUT_VARIABLE out ERROR_VARIABLE err)
  set(${result_code} "${code}" PARENT_SCOPE)
  set(${result_output} "${out}" PARENT_SCOPE)
endfunction()

# A budget condition needs a declared budget.
write_config("${work}/missing.json" "\"bm25_budget\"" "")
dry_run("${work}/missing.json" code out)
if(NOT code EQUAL 2)
  message(FATAL_ERROR "bm25_budget without context_budget_tokens must fail with exit 2, got ${code}")
endif()

# With a 50-token budget, BM25 keeps the correction that keyword top-1 misses.
write_config("${work}/budget.json" "\"retrieved_evidence\", \"bm25_budget\"" "\"context_budget_tokens\": \"50\",")
dry_run("${work}/budget.json" code out)
if(NOT code EQUAL 0)
  message(FATAL_ERROR "Budget dry run failed (${code}): ${out}")
endif()
string(JSON artifacts GET "${out}" artifact_directory)
file(READ "${artifacts}/plan.json" plan)
set(prompts "")
foreach(index 0 1)
  string(JSON condition GET "${plan}" cases ${index} condition)
  string(JSON prompt GET "${plan}" cases ${index} prompt)
  string(JSON bytes GET "${plan}" cases ${index} prompt_bytes)
  if(bytes GREATER 512)
    message(FATAL_ERROR "${condition} prompt exceeds the local 512-byte cap")
  endif()
  set(prompt_${condition} "${prompt}")
endforeach()
if(NOT prompt_bm25_budget MATCHES "corrected to")
  message(FATAL_ERROR "bm25_budget must include the correction:\n${prompt_bm25_budget}")
endif()
if(prompt_retrieved_evidence MATCHES "corrected to")
  message(FATAL_ERROR "Keyword top-1 is expected to miss the correction for seed 42")
endif()
file(READ "${artifacts}/manifest.json" manifest)
if(NOT manifest MATCHES "context_budget_tokens")
  message(FATAL_ERROR "The manifest must record the budget")
endif()
# A benchmark-only local server is not bound by the Dumas compact profile: the
# standard prompt accepts distractors and fits full history in a large context.
set(profile "standard")
set(input_bound "30000")
write_config("${work}/standard.json" "\"full_history\", \"bm25_budget\"" "\"context_budget_tokens\": \"50\",")
dry_run("${work}/standard.json" code out)
if(NOT code EQUAL 0)
  message(FATAL_ERROR "Standard local profile with distractors must plan (${code}): ${out}")
endif()
string(JSON artifacts GET "${out}" artifact_directory)
file(READ "${artifacts}/plan.json" plan)
string(JSON full GET "${plan}" cases 0 prompt)
string(JSON budget_bytes GET "${plan}" cases 1 prompt_bytes)
string(JSON full_bytes GET "${plan}" cases 0 prompt_bytes)
if(NOT full MATCHES "Visitor" OR NOT budget_bytes LESS full_bytes)
  message(FATAL_ERROR "Full history must include the distractors and exceed the budgeted prompt")
endif()
# One LongMemEval question per run: no fresh case, and known evidence is the
# question's labelled evidence messages.
file(SHA256 "${LTM_FIXTURE}" fixture_sha)
set(public_field "\"include_fresh\": false, \"max_candidate_answer_bytes\": \"8192\", \"public_case\": {\"path\": \"${LTM_FIXTURE}\", \"question_id\": \"fictional-school-01\", \"source_uri\": \"fixture\", \"revision\": \"test\", \"sha256\": \"${fixture_sha}\"}, \"context_budget_tokens\": \"50\",")
write_config("${work}/public.json" "\"no_memory\", \"known_evidence\", \"bm25_budget\"" "${public_field}")
dry_run("${work}/public.json" code out)
if(NOT code EQUAL 0)
  message(FATAL_ERROR "A public-only run must plan (${code}): ${out}")
endif()
string(JSON artifacts GET "${out}" artifact_directory)
file(READ "${artifacts}/plan.json" plan)
string(JSON count LENGTH "${plan}" cases)
string(JSON first GET "${plan}" cases 0 case_id)
string(JSON known GET "${plan}" cases 1 prompt)
string(JSON judges GET "${plan}" planned_judge_requests)
if(NOT count EQUAL 3 OR NOT first STREQUAL "public:no_memory" OR NOT judges EQUAL 3)
  message(FATAL_ERROR "Expected three public cases and three judges, got ${count} cases, first ${first}, ${judges} judges")
endif()
if(NOT known MATCHES "corrected" OR NOT known MATCHES "83")
  message(FATAL_ERROR "Known evidence must hold the labelled evidence messages:\n${known}")
endif()
set(no_public "\"include_fresh\": false,")
write_config("${work}/empty.json" "\"no_memory\"" "${no_public}")
dry_run("${work}/empty.json" code out)
if(NOT code EQUAL 2)
  message(FATAL_ERROR "include_fresh false without a public case must fail with exit 2, got ${code}")
endif()
# Dense and hybrid budgets read precomputed vectors from an embedding cache.
set(dense_field "\"include_fresh\": false, \"max_candidate_answer_bytes\": \"8192\", \"public_case\": {\"path\": \"${LTM_FIXTURE}\", \"question_id\": \"fictional-school-01\", \"source_uri\": \"fixture\", \"revision\": \"test\", \"sha256\": \"${fixture_sha}\"}, \"context_budget_tokens\": \"50\",")
write_config("${work}/dense-nocache.json" "\"dense_budget\"" "${dense_field}")
dry_run("${work}/dense-nocache.json" code out)
if(NOT code EQUAL 2 OR NOT out MATCHES "configuration")
  message(FATAL_ERROR "dense_budget without embedding_cache must fail configuration, got ${code}: ${out}")
endif()
file(MAKE_DIRECTORY "${work}/empty-cache")
write_config("${work}/dense-empty.json" "\"hybrid_budget\"" "${dense_field} \"embedding_cache\": \"empty-cache\",")
dry_run("${work}/dense-empty.json" code out)
if(NOT code EQUAL 2 OR NOT out MATCHES "prompt_preflight")
  message(FATAL_ERROR "hybrid_budget with missing vectors must fail at preflight, got ${code}: ${out}")
endif()
message(STATUS "bm25_budget checks passed")
