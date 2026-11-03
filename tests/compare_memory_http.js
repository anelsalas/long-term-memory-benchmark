'use strict';

const assert = require('node:assert/strict');
const crypto = require('node:crypto');
const fs = require('node:fs');
const http = require('node:http');
const os = require('node:os');
const path = require('node:path');
const { spawn } = require('node:child_process');

async function main() {
  const [executable, fixture] = process.argv.slice(2);
  const directory = fs.mkdtempSync(path.join(os.tmpdir(), 'ltm-memory-compare-'));
  const server = http.createServer(async (request, response) => {
    response.setHeader('content-type', 'application/json');
    if (request.url === '/config') return response.end(JSON.stringify({ config_sha256: 'fixture-hash', model: 'fixture' }));
    if (request.url === '/search') {
      let body = '';
      for await (const chunk of request) body += chunk;
      const input = JSON.parse(body);
      assert.equal(input.namespace, 'case1');
      return response.end(JSON.stringify([{ memory_id: 'memory-1', text: 'Lina corrected score: 91',
        metadata: { session_id: 'session-000002', session_date: '2026/03/12 (Thu) 10:00' } }]));
    }
    response.writeHead(404).end();
  });
  await new Promise((resolve) => server.listen(0, '127.0.0.1', resolve));
  try {
    const sha = crypto.createHash('sha256').update(fs.readFileSync(fixture)).digest('hex');
    const config = {
      schema_version: 1, source_revision: 'test', endpoint_url: 'http://127.0.0.1:8766/v1/chat/completions',
      execution_mode: 'local', prompt_profile: 'standard', conditions: ['memory_budget'],
      memory_service_url: `http://127.0.0.1:${server.address().port}`, memory_namespace: 'case1',
      context_budget_tokens: 50, reader: { id: 'r', version: 'r' }, judge: { id: 'r', version: 'r' },
      provider_policy: { max_requests: '8', max_tokens: '4096', max_cost_nanos: '0', input_token_price_nanos: '0',
        output_token_price_nanos: '0', request_price_nanos: '0', max_prompt_bytes: '4000000',
        max_response_bytes: '1000000', max_attempts: 1 },
      input_token_upper_bound: '30000', context_token_limit: '32768', output_token_limit: '500',
      judge_output_token_limit: '32', seed: '42', top_k: 5, timeout_ms: 60000, output_token_parameter: 'max_tokens',
      temperature: null, output_directory: 'artifacts', chat_framing_token_allowance: 32,
      tokenizer_bound_verified: true, include_fresh: false, max_candidate_answer_bytes: '8192',
      public_case: { path: fixture, question_id: 'fictional-school-01', source_uri: 'fixture', revision: 'test', sha256: sha },
    };
    const configPath = path.join(directory, 'config.json');
    fs.writeFileSync(configPath, JSON.stringify(config));
    const result = await new Promise((resolve, reject) => {
      const child = spawn(executable, ['--config', configPath, '--dry-run']);
      let output = '', error = '';
      child.stdout.on('data', (data) => { output += data; });
      child.stderr.on('data', (data) => { error += data; });
      child.on('error', reject);
      child.on('close', (code) => resolve({ code, output, error }));
    });
    assert.equal(result.code, 0, result.error + result.output);
    const report = JSON.parse(result.output);
    const plan = JSON.parse(fs.readFileSync(path.join(report.artifact_directory, 'plan.json')));
    const manifest = JSON.parse(fs.readFileSync(path.join(report.artifact_directory, 'manifest.json')));
    assert.match(plan.cases[0].prompt, /corrected score: 91/);
    assert.match(plan.cases[0].prompt, /\[2026\/03\/12 \(Thu\) 10:00\] Lina corrected score: 91/);
    assert.doesNotMatch(plan.cases[0].prompt, /memory-1|session-000002|"sessions"/);
    assert.deepEqual(plan.cases[0].source_ids, []);
    assert.deepEqual(plan.cases[0].memory_ids, ['memory-1']);
    assert.deepEqual(plan.cases[0].memory_source_session_ids, ['session-000002']);
    assert.equal(plan.cases[0].memory_provenance_complete, true);
    assert.equal(plan.cases[0].memory_all_evidence_sessions_found, false);
    assert.match(JSON.stringify(manifest), /fixture-hash/);
  } finally {
    await new Promise((resolve) => server.close(resolve));
    const tempRoot = fs.realpathSync(os.tmpdir());
    const resolved = fs.realpathSync(directory);
    if (path.dirname(resolved).toLowerCase() !== tempRoot.toLowerCase() ||
        !path.basename(resolved).startsWith('ltm-memory-compare-')) throw new Error('Unsafe cleanup path');
    fs.rmSync(resolved, { recursive: true, force: true });
  }
}

main().catch((error) => { console.error(error); process.exitCode = 1; });
