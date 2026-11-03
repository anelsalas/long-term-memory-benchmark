'use strict';

const fs = require('node:fs');
const { loopbackEndpoint } = require('./memory');

async function ingestSessions(payload, endpoint, outputPath) {
  const base = loopbackEndpoint(endpoint);
  if (!payload || typeof payload.namespace !== 'string' || !/^[A-Za-z0-9_-]{1,100}$/.test(payload.namespace) ||
      !Array.isArray(payload.sessions) || !payload.sessions.length || !outputPath ||
      payload.sessions.some((session) => !session || typeof session.session_id !== 'string' ||
        !/^[A-Za-z0-9_-]{1,100}$/.test(session.session_id))) throw new Error('Invalid ingestion input');
  const out = fs.openSync(outputPath, 'wx');
  let completed = 0;
  try {
    for (const session of payload.sessions) {
      const started = Date.now();
      const response = await fetch(base + '/ingest', { method: 'POST', headers: { 'content-type': 'application/json' },
        body: JSON.stringify({ namespace: payload.namespace, sessions: [session] }),
        redirect: 'error', signal: AbortSignal.timeout(30 * 60 * 1000) });
      if (!response.ok) throw new Error(`Ingestion returned HTTP ${response.status}`);
      const data = await response.json();
      const count = (value) => Number.isSafeInteger(value) && value >= 0 ? value : null;
      const promptHashes = Array.isArray(data.system_prompt_sha256) ?
        data.system_prompt_sha256.filter((hash) => typeof hash === 'string' && /^[0-9a-f]{64}$/.test(hash)) : [];
      const record = { session_id: session.session_id, memories: count(data.memories), llm_calls: count(data.llm_calls),
        input_tokens: count(data.input_tokens), output_tokens: count(data.output_tokens),
        usage_complete: data.usage_complete === true, system_prompt_sha256: promptHashes,
        wall_ms: count(data.wall_ms), client_wall_ms: Date.now() - started };
      fs.writeSync(out, JSON.stringify(record) + '\n');
      fs.fsyncSync(out);
      ++completed;
    }
    return { completed };
  } finally {
    fs.closeSync(out);
  }
}

async function main() {
  const [inputPath, endpoint, outputPath] = process.argv.slice(2);
  if (!inputPath || !endpoint || !outputPath || process.argv.length !== 5)
    throw new Error('Usage: node ingest.js INPUT_JSON SERVICE_URL OUTPUT_JSONL');
  if (fs.statSync(inputPath).size > 16 * 1024 * 1024) throw new Error('Ingestion input is too large');
  const payload = JSON.parse(fs.readFileSync(inputPath, 'utf8'));
  const result = await ingestSessions(payload, endpoint, outputPath);
  console.log(JSON.stringify(result));
}

if (require.main === module) {
  main().catch(() => { console.error('Mem0 ingestion stopped; inspect the saved progress and service state.'); process.exitCode = 1; });
}
module.exports = { ingestSessions };
