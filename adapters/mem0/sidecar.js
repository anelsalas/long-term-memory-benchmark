'use strict';

const crypto = require('node:crypto');
const fs = require('node:fs');
const http = require('node:http');
const path = require('node:path');
const Database = require('better-sqlite3');
const { openMemory } = require('./memory');

function digest(value) {
  return crypto.createHash('sha256').update(value).digest('hex');
}

function namespaceOf(value) {
  if (typeof value !== 'string' || !/^[A-Za-z0-9_-]{1,100}$/.test(value)) throw new Error('Invalid namespace');
  return value;
}

function normalizedSession(session) {
  if (!session || typeof session.session_id !== 'string' || !session.session_id ||
      typeof session.date !== 'string' || !session.date || !Array.isArray(session.rounds) || !session.rounds.length)
    throw new Error('Invalid session');
  return { session_id: session.session_id, date: session.date, rounds: session.rounds.map((round) => {
    if (!round || typeof round.round_id !== 'string' || !round.round_id || typeof round.user !== 'string' ||
        !Array.isArray(round.replies) || round.replies.some((reply) => typeof reply !== 'string'))
      throw new Error('Invalid round');
    return { round_id: round.round_id, user: round.user, replies: round.replies };
  }) };
}

function createService({ runtimeDir, config, memoryFactory = openMemory }) {
  const configHash = digest(JSON.stringify(config));
  fs.mkdirSync(runtimeDir, { recursive: true });
  const ledger = new Database(path.join(runtimeDir, 'ledger.db'));
  ledger.exec(`CREATE TABLE IF NOT EXISTS sessions (
    config_hash TEXT NOT NULL, namespace TEXT NOT NULL, session_id TEXT NOT NULL,
    content_hash TEXT NOT NULL, state TEXT NOT NULL, memories INTEGER,
    PRIMARY KEY (config_hash, namespace, session_id))`);
  const find = ledger.prepare('SELECT content_hash, state, memories FROM sessions WHERE config_hash = ? AND namespace = ? AND session_id = ?');
  const begin = ledger.prepare('INSERT INTO sessions VALUES (?, ?, ?, ?, ?, NULL)');
  const finish = ledger.prepare("UPDATE sessions SET state = 'done', memories = ? WHERE config_hash = ? AND namespace = ? AND session_id = ?");
  const stores = new Map();
  let queue = Promise.resolve();
  function store(namespace) {
    if (!stores.has(namespace)) {
      const dbPath = path.join(runtimeDir, `${configHash}-${namespace}.db`);
      stores.set(namespace, memoryFactory({ ...config, dbPath }));
    }
    return stores.get(namespace);
  }
  async function ingestRequest(request) {
    const started = Date.now();
    const namespace = namespaceOf(request.namespace);
    if (!Array.isArray(request.sessions) || !request.sessions.length) throw new Error('Invalid sessions');
    const sessions = request.sessions.map(normalizedSession);
    let memories = 0;
    for (const session of sessions) {
      const hash = digest(JSON.stringify(session));
      const previous = find.get(configHash, namespace, session.session_id);
      if (previous) {
        if (previous.content_hash !== hash) throw new Error('Session content conflict');
        if (previous.state !== 'done') throw new Error('Session outcome uncertain; inspect before retry');
        memories += previous.memories;
        continue;
      }
      // Record intent first: a crash after Mem0 writes is conservatively uncertain.
      begin.run(configHash, namespace, session.session_id, hash, 'pending');
      const messages = [];
      for (const round of session.rounds) {
        messages.push({ role: 'user', content: `[Session date: ${session.date}] ${round.user}` });
        for (const reply of round.replies) messages.push({ role: 'assistant', content: reply });
      }
      const result = await store(namespace).add(messages, { userId: namespace, metadata: {
        session_id: session.session_id, session_date: session.date,
        round_ids: session.rounds.map((round) => round.round_id),
      } });
      const count = Array.isArray(result.results) ? result.results.length : 0;
      finish.run(count, configHash, namespace, session.session_id);
      memories += count;
    }
    return { memories, llm_calls: null, input_tokens: null, output_tokens: null, wall_ms: Date.now() - started };
  }
  return {
    ingest(request) {
      const next = queue.catch(() => {}).then(() => ingestRequest(request));
      queue = next;
      return next;
    },
    async search(request) {
      const namespace = namespaceOf(request.namespace);
      if (typeof request.query !== 'string' || !request.query || !Number.isInteger(request.limit) ||
          request.limit < 1 || request.limit > 100) throw new Error('Invalid search');
      await queue.catch(() => {});
      const result = await store(namespace).search(request.query, { filters: { user_id: namespace }, topK: request.limit });
      return result.results.map((item) => ({ memory_id: item.id, text: item.memory, score: item.score,
        created_at: item.createdAt, metadata: item.metadata }));
    },
    config() { return { ...config, config_sha256: configHash }; },
    close() {
      for (const instance of stores.values()) {
        if (instance.vectorStore && instance.vectorStore.db) instance.vectorStore.db.close();
        if (instance._entityStore && instance._entityStore.db) instance._entityStore.db.close();
      }
      ledger.close();
    },
  };
}

function createHttpServer(service) {
  return http.createServer(async (request, response) => {
    function send(status, body) {
      response.writeHead(status, { 'content-type': 'application/json', 'cache-control': 'no-store' });
      response.end(JSON.stringify(body));
    }
    const port = request.socket.localPort;
    if (request.socket.remoteAddress !== '127.0.0.1' || request.headers.origin !== undefined ||
        ![`127.0.0.1:${port}`, `localhost:${port}`].includes(request.headers.host))
      return send(403, { error: 'forbidden' });
    if (request.method === 'GET' && request.url === '/health') return send(200, { status: 'ok' });
    if (request.method === 'GET' && request.url === '/config') return send(200, service.config());
    if (request.method !== 'POST' || !['/ingest', '/search'].includes(request.url))
      return send(404, { error: 'not_found' });
    try {
      let size = 0;
      const chunks = [];
      for await (const chunk of request) {
        size += chunk.length;
        if (size > 16 * 1024 * 1024) return send(413, { error: 'too_large' });
        chunks.push(chunk);
      }
      const body = JSON.parse(Buffer.concat(chunks).toString('utf8'));
      return send(200, await (request.url === '/ingest' ? service.ingest(body) : service.search(body)));
    } catch (error) {
      if (/conflict|uncertain/i.test(error.message)) return send(409, { error: 'session_conflict_or_uncertain' });
      if (error instanceof SyntaxError || /^Invalid /.test(error.message)) return send(400, { error: 'invalid_request' });
      return send(500, { error: 'operation_failed' });
    }
  });
}

module.exports = { createService, createHttpServer };
