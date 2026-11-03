'use strict';

const crypto = require('node:crypto');
const fs = require('node:fs');
const { loopbackEndpoint } = require('./memory');
const { createService, createHttpServer } = require('./sidecar');

function normalizeConfig(value) {
  const allowed = ['model', 'llmBaseURL', 'embeddingBaseURL', 'embeddingModel', 'dimensions'];
  if (!value || typeof value !== 'object' || Array.isArray(value) ||
      Object.keys(value).some((key) => !allowed.includes(key))) throw new Error('Invalid configuration field');
  if (typeof value.model !== 'string' || !/^gpt-[a-z0-9][a-z0-9./-]*$/i.test(value.model))
    throw new Error('A GPT model profile is required');
  if (typeof value.embeddingModel !== 'string' || !value.embeddingModel ||
      !Number.isInteger(value.dimensions) || value.dimensions <= 0) throw new Error('Invalid embedding configuration');
  return {
    model: value.model,
    llmBaseURL: loopbackEndpoint(value.llmBaseURL),
    embeddingBaseURL: loopbackEndpoint(value.embeddingBaseURL),
    embeddingModel: value.embeddingModel,
    dimensions: value.dimensions,
    mem0_version: '3.3.1',
    mem0_oss_bundle_sha256: crypto.createHash('sha256').update(fs.readFileSync(require.resolve('mem0ai/oss'))).digest('hex'),
  };
}

function main() {
  const [configFile, runtimeDir, portText] = process.argv.slice(2);
  const port = Number(portText);
  if (!configFile || !runtimeDir || !Number.isInteger(port) || port < 1 || port > 65535)
    throw new Error('Usage: node server.js CONFIG_FILE RUNTIME_DIR PORT');
  const config = normalizeConfig(JSON.parse(fs.readFileSync(configFile, 'utf8')));
  // Snapshot deltas are attributable to this service when it has exclusive use
  // of the bridge during ingestion. Failed attempts may have unreported tokens.
  const metricsUrl = new URL('/metrics', config.llmBaseURL);
  const metricsProvider = async () => {
    const response = await fetch(metricsUrl, { signal: AbortSignal.timeout(5000) });
    if (!response.ok) throw new Error('Bridge metrics unavailable');
    return response.json();
  };
  const service = createService({ runtimeDir, config, metricsProvider });
  const server = createHttpServer(service);
  server.listen(port, '127.0.0.1', () => console.log(`Mem0 service listening on 127.0.0.1:${port}`));
  function stop() {
    server.close(() => { service.close(); });
  }
  process.once('SIGINT', stop);
  process.once('SIGTERM', stop);
}

if (require.main === module) {
  try { main(); } catch (error) { console.error(error.message); process.exitCode = 1; }
}
module.exports = { normalizeConfig };
