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
  file(WRITE "${path}" "{
  \"schema_version\": 1, \"source_revision\": \"test\",
  \"endpoint_url\": \"http://127.0.0.1:8766/v1/chat/completions\",
  \"execution_mode\": \"local\", \"prompt_profile\": \"compact_local_smoke\", \"fresh_distractors\": 6,
  \"conditions\": [${conditions}], ${budget_field}
  \"reader\": {\"id\": \"r\", \"version\": \"r\"}, \"judge\": {\"id\": \"r\", \"version\": \"r\"},
  \"provider_policy\": {\"max_requests\": \"8\", \"max_tokens\": \"4096\", \"max_cost_nanos\": \"0\",
    \"input_token_price_nanos\": \"0\", \"output_token_price_nanos\": \"0\", \"request_price_nanos\": \"0\",
    \"max_prompt_bytes\": \"4000000\", \"max_response_bytes\": \"1000000\", \"max_attempts\": 1},
  \"input_token_upper_bound\": \"512\", \"context_token_limit\": \"1024\", \"output_token_limit\": \"500\",
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
message(STATUS "bm25_budget checks passed")
