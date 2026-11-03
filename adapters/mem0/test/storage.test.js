'use strict';

const assert = require('node:assert/strict');
const fs = require('node:fs');
const os = require('node:os');
const path = require('node:path');
const test = require('node:test');

process.env.MEM0_TELEMETRY = 'false';
const { MemoryVectorStore } = require('mem0ai/oss');

test('Mem0 persists vectors and retrieves the matching stored memory', async () => {
  const folder = fs.mkdtempSync(path.join(os.tmpdir(), 'ltm-mem0-test-'));
  const dbPath = path.join(folder, 'vectors.sqlite');
  let store;
  try {
    store = new MemoryVectorStore({ dbPath, dimension: 2 });
    await store.insert([[1, 0], [0, 1]], ['first', 'second'], [
      { data: 'Updated grade is 82', user_id: 'case-a' },
      { data: 'A different topic', user_id: 'case-a' },
    ]);
    store.db.close();
    store = new MemoryVectorStore({ dbPath, dimension: 2 });
    const results = await store.search([1, 0], 2, { user_id: 'case-a' });
    assert.equal(results[0].id, 'first');
    assert.equal(results[0].payload.data, 'Updated grade is 82');
  } finally {
    if (store) store.db.close();
    fs.rmSync(dbPath, { force: true });
    fs.rmdirSync(folder);
  }
});
