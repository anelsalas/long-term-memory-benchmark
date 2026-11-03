'use strict';

// Set before importing Mem0: the upstream module reads this flag at load time.
process.env.MEM0_TELEMETRY = 'false';
const { Memory } = require('mem0ai/oss');

function loopbackEndpoint(value) {
  const url = new URL(value);
  if (url.protocol !== 'http:' || !['127.0.0.1', 'localhost'].includes(url.hostname) ||
      url.username || url.password) throw new Error('Provider must use a loopback HTTP endpoint');
  return url.toString().replace(/\/$/, '');
}

function openMemory({ dbPath, model, llmBaseURL, embeddingBaseURL, embeddingModel, dimensions }) {
  // Mem0 3.3.1 derives its entity-store filename by replacing a .db suffix.
  if (!dbPath || !dbPath.endsWith('.db') || !model || !embeddingModel || !Number.isInteger(dimensions) || dimensions <= 0)
    throw new Error('Invalid memory configuration');
  return new Memory({
    vectorStore: { provider: 'memory', config: { dbPath, dimension: dimensions } },
    llm: { provider: 'openai', config: { apiKey: 'local-only', model, baseURL: loopbackEndpoint(llmBaseURL) } },
    embedder: { provider: 'openai', config: {
      apiKey: 'local-only', model: embeddingModel, baseURL: loopbackEndpoint(embeddingBaseURL),
      embeddingDims: dimensions,
    } },
    disableHistory: true,
  });
}

module.exports = { openMemory, loopbackEndpoint };
