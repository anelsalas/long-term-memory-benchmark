'use strict';

const assert = require('node:assert/strict');
const fs = require('node:fs');
const http = require('node:http');
const os = require('node:os');
const path = require('node:path');
const test = require('node:test');
const { ingestSessions } = require('../ingest');

test('ingestion runner sends one session at a time and writes dialogue-free progress', async () => {
  const folder = fs.mkdtempSync(path.join(os.tmpdir(), 'ltm-ingest-runner-'));
  const output = path.join(folder, 'progress.jsonl');
  const seen = [];
  const server = http.createServer(async (request, response) => {
    let body = '';
    for await (const chunk of request) body += chunk;
    const data = JSON.parse(body);
    seen.push(data.sessions[0].session_id);
    response.setHeader('content-type', 'application/json');
    response.end(JSON.stringify({ memories: 1, llm_calls: 1, input_tokens: 5, output_tokens: 2,
      usage_complete: true, wall_ms: 10 }));
  });
  await new Promise((resolve) => server.listen(0, '127.0.0.1', resolve));
  try {
    const payload = { namespace: 'case1', sessions: [
      { session_id: 's1', date: '2025-01-01', rounds: [{ round_id: 'r1', user: 'PRIVATE FIXTURE', replies: [] }] },
      { session_id: 's2', date: '2025-01-02', rounds: [{ round_id: 'r2', user: 'ANOTHER FIXTURE', replies: [] }] },
    ] };
    const result = await ingestSessions(payload, `http://127.0.0.1:${server.address().port}`, output);
    const lines = fs.readFileSync(output, 'utf8').trim().split('\n').map(JSON.parse);
    assert.deepEqual(seen, ['s1', 's2']);
    assert.equal(result.completed, 2);
    assert.equal(lines.length, 2);
    assert.equal(lines[0].session_id, 's1');
    assert.equal(lines[0].llm_calls, 1);
    assert.doesNotMatch(fs.readFileSync(output, 'utf8'), /PRIVATE FIXTURE|ANOTHER FIXTURE/);
    await assert.rejects(ingestSessions(payload, `http://127.0.0.1:${server.address().port}`, output), /EEXIST/);
  } finally {
    await new Promise((resolve) => server.close(resolve));
    fs.rmSync(output, { force: true });
    fs.rmdirSync(folder);
  }
});

test('ingestion runner stops after an error and keeps completed progress', async () => {
  const folder = fs.mkdtempSync(path.join(os.tmpdir(), 'ltm-ingest-runner-'));
  const output = path.join(folder, 'progress.jsonl');
  let calls = 0;
  const server = http.createServer(async (request, response) => {
    for await (const unused of request) { void unused; }
    if (++calls === 2) return response.writeHead(503).end();
    response.setHeader('content-type', 'application/json');
    response.end(JSON.stringify({ memories: 1, llm_calls: 1, input_tokens: 5, output_tokens: 2,
      usage_complete: true, wall_ms: 10 }));
  });
  await new Promise((resolve) => server.listen(0, '127.0.0.1', resolve));
  try {
    const payload = { namespace: 'case1', sessions: ['s1', 's2', 's3'].map((session_id) => ({ session_id })) };
    await assert.rejects(ingestSessions(payload, `http://127.0.0.1:${server.address().port}`, output), /503/);
    assert.equal(calls, 2);
    assert.equal(fs.readFileSync(output, 'utf8').trim().split('\n').length, 1);
  } finally {
    await new Promise((resolve) => server.close(resolve));
    fs.rmSync(output, { force: true });
    fs.rmdirSync(folder);
  }
});
