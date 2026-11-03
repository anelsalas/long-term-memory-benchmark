'use strict';

const assert = require('node:assert/strict');
const fs = require('node:fs');
const http = require('node:http');
const os = require('node:os');
const path = require('node:path');
const test = require('node:test');

process.env.MEM0_TELEMETRY = 'false';
const { openMemory } = require('../memory');

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
  const dbPath = path.join(folder, 'vectors.db');
  const entityPath = path.join(folder, 'vectors_entities.db');
  let memory;
  try {
    const baseURL = `http://127.0.0.1:${server.address().port}/v1`;
    memory = openMemory({ dbPath, model: 'fake', llmBaseURL: baseURL, embeddingBaseURL: baseURL,
      embeddingModel: 'fake-embedding', dimensions: 2 });
    await memory.add([{ role: 'user', content: 'On 2025-02-12, Lina’s grade was corrected to 82.' }],
      { userId: 'fixture', metadata: { session_id: 'session-1', session_date: '2025-02-12' } });
    const result = await memory.search('What was Lina’s corrected grade?',
      { filters: { user_id: 'fixture' }, topK: 5 });
    assert.ok(result.results.some((item) => item.memory.includes('82')));
    assert.ok(result.results.some((item) => item.metadata.session_id === 'session-1'));
    const chat = calls.find((call) => call.url === '/v1/chat/completions');
    assert.equal(chat.body.messages[0].role, 'system');
    assert.equal(chat.body.messages[1].role, 'user');
  } finally {
    if (memory && memory.vectorStore) memory.vectorStore.db.close();
    if (memory && memory._entityStore) memory._entityStore.db.close();
    await new Promise((resolve) => server.close(resolve));
    fs.rmSync(dbPath, { force: true });
    fs.rmSync(entityPath, { force: true });
    fs.rmdirSync(folder);
  }
});
