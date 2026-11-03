'use strict';

const assert = require('node:assert/strict');
const test = require('node:test');
const { summarize } = require('../report');

test('report compares one case and counts replayed ingestion sessions once', () => {
  const progressRuns = [
    [{ session_id: 's1', memories: 2, llm_calls: 1, input_tokens: 10, output_tokens: 3,
      usage_complete: true, client_wall_ms: 1000, system_prompt_sha256: ['a'.repeat(64)] }],
    [{ session_id: 's1', memories: 2, llm_calls: 1, input_tokens: 10, output_tokens: 3,
      usage_complete: true, client_wall_ms: 5, system_prompt_sha256: ['a'.repeat(64)] },
    { session_id: 's2', memories: 3, llm_calls: 1, input_tokens: 20, output_tokens: 5,
      usage_complete: true, client_wall_ms: 2000, system_prompt_sha256: ['a'.repeat(64)] }],
  ];
  const result = summarize({ caseId: 'case1', expectedSessions: 2, progressRuns,
    record: { case_id: 'public:memory_budget', correct: true, status: 'completed', answer: 'PRIVATE ANSWER' },
    detail: { reader_wall_ms: 80, retrieval_preparation_ms: 10, memory_all_evidence_sessions_found: true,
      attempts: [{ usage: { input_tokens: 100, output_tokens: 8 }, prompt: 'PRIVATE PROMPT' }],
      judge: { attempts: [{ usage: { input_tokens: 25, output_tokens: 2 }, reply: 'PRIVATE JUDGE' }] } },
    baselineCases: [{ question_id: 'case1', reader: 'GPT-6.1-sol', condition: 'dense_budget',
      correct: false, input_tokens: 120, output_tokens: 9, reader_wall_ms: 70 }],
    resourceSamples: [
      { timestamp_utc: '2026-11-03T12:00:00Z', processes: { embedding: {
        cpu_total_seconds: 10, working_set_bytes: 1000, private_bytes: 800 } } },
      { timestamp_utc: '2026-11-03T12:00:05Z', processes: { embedding: {
        cpu_total_seconds: 12, working_set_bytes: 1500, private_bytes: 900 } } },
    ],
  });
  assert.equal(result.ingestion.sessions, 2);
  assert.equal(result.ingestion.llm_calls, 2);
  assert.equal(result.ingestion.input_tokens, 30);
  assert.equal(result.ingestion.observed_session_wall_ms, 3000);
  assert.equal(result.mem0.correct, true);
  assert.equal(result.dense_baseline.correct, false);
  assert.equal(result.local_resources.processes.embedding.cpu_delta_seconds, 2);
  assert.equal(result.local_resources.processes.embedding.sampled_peak_working_set_bytes, 1500);
  assert.doesNotMatch(JSON.stringify(result), /PRIVATE/);
});

test('report refuses an incomplete case', () => {
  assert.throws(() => summarize({ caseId: 'case1', expectedSessions: 2, progressRuns: [[]],
    record: {}, detail: {}, baselineCases: [] }), /incomplete/i);
});
