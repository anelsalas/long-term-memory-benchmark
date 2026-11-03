'use strict';

const assert = require('node:assert/strict');
const fs = require('node:fs');
const http = require('node:http');
const os = require('node:os');
const path = require('node:path');
const test = require('node:test');
const { createService, createHttpServer } = require('../sidecar');
const { normalizeConfig } = require('../server');

test('startup accepts an explicit GPT profile and rejects other providers or extra fields', () => {
  const config = { model: 'gpt-6.1-sol/high', llmBaseURL: 'http://127.0.0.1:8773/v1',
    embeddingBaseURL: 'http://127.0.0.1:8771/v1', embeddingModel: 'qwen3-embedding', dimensions: 1024 };
  assert.equal(normalizeConfig(config).model, config.model);
  assert.throws(() => normalizeConfig({ ...config, model: 'claude-model' }), /GPT/);
  assert.throws(() => normalizeConfig({ ...config, apiKey: 'unneeded' }), /field/);
});

test('sidecar ingests sessions once, preserves dates, and returns ranked memories', async () => {
  const folder = fs.mkdtempSync(path.join(os.tmpdir(), 'ltm-sidecar-'));
  const calls = [];
  let attempts = 0;
  const metricsProvider = async () => ({ instance_id: 'bridge-1', attempts, completed: attempts, input_tokens: attempts * 20,
    output_tokens: attempts * 5, missing_usage: 0 });
  const service = createService({ runtimeDir: folder, config: { version: 'test' }, metricsProvider, memoryFactory: () => ({
    async add(messages, options) { ++attempts; calls.push({ messages, options }); return { results: [{ memory: 'grade 82' }] }; },
    async search() { return { results: [{ id: 'm1', memory: 'grade 82', score: 0.9,
      metadata: { session_id: 's1', session_date: '2025-02-12' } }] }; },
  }) });
  const session = { session_id: 's1', date: '2025-02-12', leading_assistant: ['Welcome back.'], rounds: [
    { round_id: 'r1', user: 'The grade is 82.', replies: ['Noted.'] },
  ] };
  try {
    const first = await service.ingest({ namespace: 'case1', sessions: [session] });
    await service.ingest({ namespace: 'case1', sessions: [session] });
    assert.equal(first.memories, 1);
    assert.equal(first.llm_calls, 1);
    assert.equal(first.input_tokens, 20);
    assert.equal(first.output_tokens, 5);
    assert.equal(first.usage_complete, true);
    assert.equal(calls.length, 1);
    assert.equal(calls[0].options.metadata.session_date, session.date);
    assert.equal(calls[0].messages[0].role, 'assistant');
    assert.match(calls[0].messages[0].content, /2025-02-12.*Welcome back/);
    const found = await service.search({ namespace: 'case1', query: 'grade', limit: 5 });
    assert.equal(found[0].text, 'grade 82');
    assert.equal(found[0].metadata.session_id, 's1');
    await assert.rejects(service.ingest({ namespace: 'case1', sessions: [{ ...session, date: '2025-02-13' }] }),
      /conflict/i);
    await assert.rejects(service.search({ namespace: '../escape', query: 'grade', limit: 5 }), /namespace/i);
  } finally {
    service.close();
    fs.rmSync(path.join(folder, 'ledger.db'), { force: true });
    fs.rmdirSync(folder);
  }
});

test('sidecar blocks an uncertain interrupted ingestion from being repeated', async () => {
  const folder = fs.mkdtempSync(path.join(os.tmpdir(), 'ltm-sidecar-'));
  let calls = 0;
  const options = { runtimeDir: folder, config: { version: 'test' }, memoryFactory: () => ({
    async add() { ++calls; throw new Error('simulated interruption'); },
  }) };
  let service = createService(options);
  const session = { session_id: 's1', date: '2025-02-12', rounds: [
    { round_id: 'r1', user: 'A fact.', replies: [] },
  ] };
  try {
    await assert.rejects(service.ingest({ namespace: 'case1', sessions: [session] }));
    service.close();
    service = createService(options);
    await assert.rejects(service.ingest({ namespace: 'case1', sessions: [session] }), /uncertain/i);
    assert.equal(calls, 1);
  } finally {
    service.close();
    fs.rmSync(path.join(folder, 'ledger.db'), { force: true });
    fs.rmdirSync(folder);
  }
});

test('sidecar marks token totals incomplete when an attempt fails', async () => {
  const folder = fs.mkdtempSync(path.join(os.tmpdir(), 'ltm-sidecar-'));
  let called = false;
  const metricsProvider = async () => called
    ? { instance_id: 'bridge-1', attempts: 2, completed: 1, input_tokens: 20, output_tokens: 5, missing_usage: 0 }
    : { instance_id: 'bridge-1', attempts: 0, completed: 0, input_tokens: 0, output_tokens: 0, missing_usage: 0 };
  const service = createService({ runtimeDir: folder, config: { version: 'test' }, metricsProvider,
    memoryFactory: () => ({ async add() { called = true; return { results: [] }; } }) });
  try {
    const result = await service.ingest({ namespace: 'case1', sessions: [{ session_id: 's1', date: '2025-02-12',
      rounds: [{ round_id: 'r1', user: 'A fact.', replies: [] }] }] });
    assert.equal(result.llm_calls, 2);
    assert.equal(result.usage_complete, false);
  } finally {
    service.close();
    fs.rmSync(path.join(folder, 'ledger.db'), { force: true });
    fs.rmdirSync(folder);
  }
});

test('sidecar reports unknown usage if the bridge restarts during ingestion', async () => {
  const folder = fs.mkdtempSync(path.join(os.tmpdir(), 'ltm-sidecar-'));
  let called = false;
  const metricsProvider = async () => ({ instance_id: called ? 'bridge-2' : 'bridge-1',
    attempts: called ? 1 : 0, completed: called ? 1 : 0, input_tokens: called ? 20 : 0,
    output_tokens: called ? 5 : 0, missing_usage: 0 });
  const service = createService({ runtimeDir: folder, config: { version: 'test' }, metricsProvider,
    memoryFactory: () => ({ async add() { called = true; return { results: [] }; } }) });
  try {
    const result = await service.ingest({ namespace: 'case1', sessions: [{ session_id: 's1', date: '2025-02-12',
      rounds: [{ round_id: 'r1', user: 'A fact.', replies: [] }] }] });
    assert.equal(result.llm_calls, null);
    assert.equal(result.usage_complete, false);
  } finally {
    service.close();
    fs.rmSync(path.join(folder, 'ledger.db'), { force: true });
    fs.rmdirSync(folder);
  }
});

test('HTTP interface accepts local clients and refuses browsers and foreign hosts', async () => {
  const service = { config: () => ({ version: 'test' }), search: async () => [], ingest: async () => ({ memories: 1 }) };
  const server = createHttpServer(service);
  await new Promise((resolve) => server.listen(0, '127.0.0.1', resolve));
  const port = server.address().port;
  function send(target, headers = {}, body) {
    return new Promise((resolve, reject) => {
      const req = http.request({ host: '127.0.0.1', port, path: target,
        method: body === undefined ? 'GET' : 'POST', headers }, (res) => {
        let result = '';
        res.on('data', (chunk) => { result += chunk; });
        res.on('end', () => resolve({ status: res.statusCode, body: result }));
      });
      req.on('error', reject);
      req.end(body === undefined ? undefined : JSON.stringify(body));
    });
  }
  try {
    assert.equal((await send('/health')).status, 200);
    assert.equal(JSON.parse((await send('/config')).body).version, 'test');
    assert.equal(JSON.parse((await send('/ingest', {}, { sessions: [] })).body).memories, 1);
    assert.deepEqual(JSON.parse((await send('/search', {}, { query: 'grade' })).body), []);
    assert.equal((await send('/health', { origin: 'http://example.invalid' })).status, 403);
    assert.equal((await send('/health', { host: 'example.invalid' })).status, 403);
  } finally {
    await new Promise((resolve) => server.close(resolve));
  }
});
