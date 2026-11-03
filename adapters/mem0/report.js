'use strict';

const fs = require('node:fs');
const path = require('node:path');

function sumKnown(values) {
  if (values.some((value) => !Number.isSafeInteger(value) || value < 0)) return null;
  const sum = values.reduce((a, b) => a + b, 0);
  return Number.isSafeInteger(sum) ? sum : null;
}

function attemptUsage(attempts) {
  if (!Array.isArray(attempts) || !attempts.length) return { requests: 0, input_tokens: null, output_tokens: null };
  return { requests: attempts.length, input_tokens: sumKnown(attempts.map((a) => a.usage && a.usage.input_tokens)),
    output_tokens: sumKnown(attempts.map((a) => a.usage && a.usage.output_tokens)) };
}

function summarizeResources(samples) {
  if (!Array.isArray(samples) || !samples.length) return null;
  const processes = {};
  for (const role of ['embedding', 'bridge', 'sidecar', 'runner']) {
    const rows = samples.map((sample) => sample.processes && sample.processes[role]).filter(Boolean);
    if (!rows.length) continue;
    const cpus = rows.map((row) => row.cpu_total_seconds);
    const memory = rows.map((row) => row.working_set_bytes);
    const privateBytes = rows.map((row) => row.private_bytes);
    processes[role] = { samples: rows.length,
      cpu_delta_seconds: cpus.every((value) => Number.isFinite(value)) && cpus.at(-1) >= cpus[0]
        ? cpus.at(-1) - cpus[0] : null,
      sampled_peak_working_set_bytes: memory.every(Number.isSafeInteger) ? Math.max(...memory) : null,
      sampled_peak_private_bytes: privateBytes.every(Number.isSafeInteger) ? Math.max(...privateBytes) : null };
  }
  return { scope: 'sampled local processes only; excludes model subprocesses and remote inference',
    sampling_coverage: 'Only the recorded sampling window; compare timestamps with the experiment.', samples: samples.length,
    first_sample_utc: samples[0].timestamp_utc, last_sample_utc: samples.at(-1).timestamp_utc, processes };
}

function summarize({ caseId, expectedSessions, progressRuns, record, detail, baselineCases, resourceSamples }) {
  if (!caseId || !Number.isSafeInteger(expectedSessions) || expectedSessions < 1 || !Array.isArray(progressRuns))
    throw new Error('Invalid report inputs');
  const sessions = new Map();
  for (const run of progressRuns) for (const entry of run) {
    if (!entry || typeof entry.session_id !== 'string' || !entry.session_id) throw new Error('Invalid progress record');
    const previous = sessions.get(entry.session_id);
    if (previous) {
      for (const field of ['memories', 'llm_calls', 'input_tokens', 'output_tokens', 'usage_complete'])
        if (previous[field] !== entry[field]) throw new Error('Conflicting replay usage');
    }
    sessions.set(entry.session_id, { ...entry,
      client_wall_ms: previous ? Math.max(previous.client_wall_ms, entry.client_wall_ms) : entry.client_wall_ms });
  }
  if (sessions.size !== expectedSessions || !record || record.status !== 'completed' ||
      typeof record.correct !== 'boolean' || record.case_id !== 'public:memory_budget')
    throw new Error('Incomplete benchmark case');
  const base = baselineCases.filter((item) => item.question_id === caseId && item.reader === 'GPT-6.1-sol' &&
    item.condition === 'dense_budget');
  if (base.length !== 1 || typeof base[0].correct !== 'boolean') throw new Error('Missing dense baseline');
  const rows = [...sessions.values()];
  const hashes = [...new Set(rows.flatMap((row) => Array.isArray(row.system_prompt_sha256) ? row.system_prompt_sha256 : []))]
    .filter((hash) => typeof hash === 'string' && /^[0-9a-f]{64}$/.test(hash)).sort();
  return {
    schema_version: 1,
    question_id: caseId,
    interpretation: 'Single-case adapted comparison; system-level accuracy requires more questions.',
    ingestion: { sessions: rows.length, memory_events: sumKnown(rows.map((row) => row.memories)),
      llm_calls: sumKnown(rows.map((row) => row.llm_calls)), input_tokens: sumKnown(rows.map((row) => row.input_tokens)),
      output_tokens: sumKnown(rows.map((row) => row.output_tokens)),
      usage_complete: rows.every((row) => row.usage_complete === true),
      observed_session_wall_ms: sumKnown(rows.map((row) => row.client_wall_ms)),
      wall_time_definition: 'Sum of longest observed client time per session across progress files; excludes gaps.',
      system_prompt_sha256: hashes },
    mem0: { correct: record.correct, retrieval_preparation_ms: detail.retrieval_preparation_ms ?? null,
      reader_wall_ms: detail.reader_wall_ms ?? null, reader_usage: attemptUsage(detail.attempts),
      judge_usage: attemptUsage(detail.judge && detail.judge.attempts),
      all_evidence_sessions_found: detail.memory_all_evidence_sessions_found ?? null,
      provenance_complete: detail.memory_provenance_complete ?? null },
    dense_baseline: { correct: base[0].correct, reader_wall_ms: base[0].reader_wall_ms ?? null,
      input_tokens: base[0].input_tokens ?? null, output_tokens: base[0].output_tokens ?? null },
    local_resources: summarizeResources(resourceSamples),
  };
}

function main() {
  const [caseId, count, comparisonDir, baselineFile, outputFile, ...files] = process.argv.slice(2);
  const resourceIndex = files.indexOf('--resources');
  const resourceFile = resourceIndex < 0 ? null : files[resourceIndex + 1];
  if (resourceIndex >= 0 && (!resourceFile || resourceIndex !== files.length - 2)) throw new Error('Invalid resources argument');
  const progressFiles = resourceIndex < 0 ? files : files.slice(0, resourceIndex);
  if (!caseId || !count || !comparisonDir || !baselineFile || !outputFile || !progressFiles.length)
    throw new Error('Usage: node report.js CASE_ID SESSION_COUNT COMPARISON_DIR BASELINE_JSON OUTPUT_JSON PROGRESS_JSONL...');
  const read = (file) => JSON.parse(fs.readFileSync(file, 'utf8'));
  const results = read(path.join(comparisonDir, 'results.json'));
  const records = results.records.filter((item) => item.case_id === 'public:memory_budget');
  if (records.length !== 1) throw new Error('Incomplete benchmark case');
  const report = summarize({ caseId, expectedSessions: Number(count),
    progressRuns: progressFiles.map((file) => fs.readFileSync(file, 'utf8').split(/\r?\n/).filter(Boolean).map(JSON.parse)),
    record: records[0], detail: read(path.join(comparisonDir, 'case-0.result.json')),
    baselineCases: read(baselineFile).cases,
    resourceSamples: resourceFile ? fs.readFileSync(resourceFile, 'utf8').split(/\r?\n/).filter(Boolean).map(JSON.parse) : null });
  fs.writeFileSync(outputFile, JSON.stringify(report, null, 2) + '\n', { flag: 'wx' });
  console.log('Wrote dialogue-free single-case report.');
}

if (require.main === module) {
  try { main(); } catch (_) { console.error('Report could not be produced; verify the completed case and inputs.'); process.exitCode = 1; }
}
module.exports = { summarize };
