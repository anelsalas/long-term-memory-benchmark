'use strict';

const assert = require('node:assert/strict');
const fs = require('node:fs');
const http = require('node:http');
const os = require('node:os');
const path = require('node:path');
const test = require('node:test');

process.env.MEM0_TELEMETRY = 'false';
const { openMemory } = require('../memory');
const { createService, createHttpServer } = require('../sidecar');

test('memory configuration restricts providers to loopback and separates entity storage', () => {
  const settings = { dbPath: 'fixture.db', model: 'fake', embeddingModel: 'fake', dimensions: 2,
    llmBaseURL: 'http://127.0.0.1:8773/v1', embeddingBaseURL: 'http://127.0.0.1:8771/v1' };
  assert.throws(() => openMemory({ ...settings, llmBaseURL: 'https://example.com/v1' }), /loopback/);
  assert.throws(() => openMemory({ ...settings, embeddingBaseURL: 'http://remote.example/v1' }), /loopback/);
  assert.throws(() => openMemory({ ...settings, dbPath: 'fixture.sqlite' }), /configuration/);
});

test('Mem0 extracts and retrieves a dated fictional session through local fake providers', async () => {
  const calls = [];
  const server = http.createServer(async (request, response) => {
    let body = '';
    for await (const chunk of request) body += chunk;
    const data = JSON.parse(body);
    calls.push({ url: request.url, body: data });
    response.setHeader('content-type', 'application/json');
    if (request.url === '/v1/embeddings') {
      const inputs = Array.isArray(data.input) ? data.input : [data.input];
      response.end(JSON.stringify({ data: inputs.map((_, index) => ({ index, embedding: [1, 0] })) }));
    } else if (request.url === '/v1/chat/completions') {
      response.end(JSON.stringify({ choices: [{ message: { role: 'assistant', content:
        JSON.stringify({ memory: [{ id: '0', text: 'Lina received a corrected grade of 82.' }] }) } }] }));
    } else {
      response.writeHead(404).end();
    }
  });
  await new Promise((resolve) => server.listen(0, '127.0.0.1', resolve));
  const folder = fs.mkdtempSync(path.join(os.tmpdir(), 'ltm-mem0-ingest-'));
  let service;
  let sidecar;
  try {
    const baseURL = `http://127.0.0.1:${server.address().port}/v1`;
    service = createService({ runtimeDir: folder, config: { model: 'fake', llmBaseURL: baseURL,
      embeddingBaseURL: baseURL, embeddingModel: 'fake-embedding', dimensions: 2 } });
    sidecar = createHttpServer(service);
    await new Promise((resolve) => sidecar.listen(0, '127.0.0.1', resolve));
    const sidecarURL = `http://127.0.0.1:${sidecar.address().port}`;
    async function post(route, body) {
      const reply = await fetch(sidecarURL + route, { method: 'POST', headers: { 'content-type': 'application/json' },
        body: JSON.stringify(body) });
      assert.equal(reply.status, 200);
      return reply.json();
    }
    const request = { namespace: 'fixture', sessions: [{ session_id: 'session-1', date: '2025-02-12',
      rounds: [{ round_id: 'r1', user: 'Lina’s grade was corrected to 82.', replies: [] }] }] };
    await post('/ingest', request);
    await post('/ingest', request);
    const result = await post('/search', { namespace: 'fixture', query: 'What was Lina’s corrected grade?', limit: 5 });
    assert.ok(result.some((item) => item.text.includes('82')));
    assert.ok(result.some((item) => item.metadata.session_id === 'session-1'));
    const chat = calls.find((call) => call.url === '/v1/chat/completions');
    assert.equal(chat.body.messages[0].role, 'system');
    assert.equal(chat.body.messages[1].role, 'user');
    assert.equal(calls.filter((call) => call.url === '/v1/chat/completions').length, 1);
  } finally {
    if (sidecar) await new Promise((resolve) => sidecar.close(resolve));
    if (service) service.close();
    await new Promise((resolve) => server.close(resolve));
    for (const file of fs.readdirSync(folder)) fs.rmSync(path.join(folder, file), { force: true });
    fs.rmdirSync(folder);
  }
});
