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
      typeof session.date !== 'string' || !session.date || !Array.isArray(session.rounds) ||
      (session.leading_assistant !== undefined && (!Array.isArray(session.leading_assistant) ||
        session.leading_assistant.some((message) => typeof message !== 'string'))) ||
      (!session.rounds.length && !(session.leading_assistant || []).length))
    throw new Error('Invalid session');
  return { session_id: session.session_id, date: session.date, leading_assistant: session.leading_assistant || [],
    rounds: session.rounds.map((round) => {
    if (!round || typeof round.round_id !== 'string' || !round.round_id || typeof round.user !== 'string' ||
        !Array.isArray(round.replies) || round.replies.some((reply) => typeof reply !== 'string'))
      throw new Error('Invalid round');
    return { round_id: round.round_id, user: round.user, replies: round.replies };
  }) };
}

function createService({ runtimeDir, config, memoryFactory = openMemory, metricsProvider }) {
  const configHash = digest(JSON.stringify(config));
  fs.mkdirSync(runtimeDir, { recursive: true });
  const ledger = new Database(path.join(runtimeDir, 'ledger.db'));
  ledger.exec(`CREATE TABLE IF NOT EXISTS sessions (
    config_hash TEXT NOT NULL, namespace TEXT NOT NULL, session_id TEXT NOT NULL,
    content_hash TEXT NOT NULL, state TEXT NOT NULL, memories INTEGER, usage_json TEXT,
    PRIMARY KEY (config_hash, namespace, session_id))`);
  if (!ledger.pragma('table_info(sessions)').some((column) => column.name === 'usage_json'))
    ledger.exec('ALTER TABLE sessions ADD COLUMN usage_json TEXT');
  const find = ledger.prepare('SELECT content_hash, state, memories, usage_json FROM sessions WHERE config_hash = ? AND namespace = ? AND session_id = ?');
  const begin = ledger.prepare('INSERT INTO sessions (config_hash, namespace, session_id, content_hash, state) VALUES (?, ?, ?, ?, ?)');
  const finish = ledger.prepare("UPDATE sessions SET state = 'done', memories = ?, usage_json = ? WHERE config_hash = ? AND namespace = ? AND session_id = ?");
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
    const perSession = [];
    for (const session of sessions) {
      const hash = digest(JSON.stringify(session));
      const previous = find.get(configHash, namespace, session.session_id);
      if (previous) {
        if (previous.content_hash !== hash) throw new Error('Session content conflict');
        if (previous.state !== 'done') throw new Error('Session outcome uncertain; inspect before retry');
        memories += previous.memories;
        perSession.push({ session_id: session.session_id, ...(previous.usage_json ? JSON.parse(previous.usage_json) :
          { llm_calls: null, input_tokens: null, output_tokens: null, usage_complete: false, system_prompt_sha256: [] }) });
        continue;
      }
      const before = metricsProvider ? await metricsProvider() : null;
      // Record intent first: a crash after Mem0 writes is conservatively uncertain.
      begin.run(configHash, namespace, session.session_id, hash, 'pending');
      const messages = [];
      for (const [index, reply] of session.leading_assistant.entries())
        messages.push({ role: 'assistant', content: index === 0 ? `[Session date: ${session.date}] ${reply}` : reply });
      for (const round of session.rounds) {
        messages.push({ role: 'user', content: `[Session date: ${session.date}] ${round.user}` });
        for (const reply of round.replies) messages.push({ role: 'assistant', content: reply });
      }
      const result = await store(namespace).add(messages, { userId: namespace, metadata: {
        session_id: session.session_id, session_date: session.date,
        round_ids: session.rounds.map((round) => round.round_id),
      } });
      const count = Array.isArray(result.results) ? result.results.length : 0;
      let usage = { llm_calls: null, input_tokens: null, output_tokens: null, usage_complete: false, system_prompt_sha256: [] };
      if (before) {
        try {
          const after = await metricsProvider();
          const names = ['attempts', 'completed', 'input_tokens', 'output_tokens', 'missing_usage'];
          if (typeof before.instance_id !== 'string' || !before.instance_id || before.instance_id !== after.instance_id ||
              !names.every((name) => Number.isSafeInteger(before[name]) && Number.isSafeInteger(after[name]) &&
              before[name] >= 0 && after[name] >= before[name])) throw new Error('Invalid usage metrics');
          const attempts = after.attempts - before.attempts;
          const completed = after.completed - before.completed;
          const hashes = Array.isArray(after.system_prompt_sha256) &&
            after.system_prompt_sha256.every((hash) => typeof hash === 'string' && /^[0-9a-f]{64}$/.test(hash))
            ? after.system_prompt_sha256 : [];
          usage = { llm_calls: attempts, input_tokens: after.input_tokens - before.input_tokens,
            output_tokens: after.output_tokens - before.output_tokens,
            usage_complete: attempts === completed && after.missing_usage === before.missing_usage,
            system_prompt_sha256: hashes };
        } catch (_) {
          // Ingestion succeeded; unavailable counters remain explicitly unknown.
        }
      }
      finish.run(count, JSON.stringify(usage), configHash, namespace, session.session_id);
      memories += count;
      perSession.push({ session_id: session.session_id, ...usage });
    }
    function total(name) {
      if (perSession.some((entry) => !Number.isSafeInteger(entry[name]))) return null;
      const sum = perSession.reduce((value, entry) => value + entry[name], 0);
      return Number.isSafeInteger(sum) ? sum : null;
    }
    return { memories, llm_calls: total('llm_calls'), input_tokens: total('input_tokens'),
      output_tokens: total('output_tokens'), usage_complete: perSession.every((entry) => entry.usage_complete),
      system_prompt_sha256: [...new Set(perSession.flatMap((entry) => entry.system_prompt_sha256 || []))].sort(),
      sessions: perSession, wall_ms: Date.now() - started };
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
