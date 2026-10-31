import assert from "node:assert/strict";
import { readFile } from "node:fs/promises";

const source = await readFile(new URL("app.js", import.meta.url), "utf8");
const api = await import(`data:text/javascript;base64,${Buffer.from(source).toString("base64")}`);

assert.equal(api.integerValue("18446744073709551615"), "18446744073709551615");
assert.throws(() => api.integerValue("18446744073709551616"));
assert.throws(() => api.integerValue(-1));
assert.throws(() => api.integerValue(9007199254740992));
assert.equal(api.dollarsToNanos("0.000000001"), "1");
assert.equal(api.dollarsToNanos("12.345"), "12345000000");
assert.throws(() => api.dollarsToNanos("0.0000000001"));
assert.equal(api.nanosToDollars("12345000000"), "12.345");
assert.equal(api.measuredValue(null), "Unavailable");
assert.equal(api.measuredValue(""), "Unavailable");
assert.equal(api.measuredValue([]), "Unavailable");
assert.equal(api.measuredValue(false), "Unavailable");
assert.equal(api.measuredValue(0, "percent"), "0.0%");
assert.equal(api.measuredValue(1.1, "percent"), "Unavailable");
assert.equal(api.measuredValue("0", "cost"), "$0");
assert.equal(api.measuredValue(Number.NaN), "Unavailable");

let captured;
const fetcher = async (path, options) => {
  captured = { path, options };
  return { ok: true, headers: { get: () => "application/json" }, json: async () => ({ schema_version: 1 }) };
};
await api.apiRequest("/api/status", {}, {}, fetcher);
assert.equal(captured.options.credentials, "same-origin");
assert.equal(captured.options.cache, "no-store");
assert.equal(captured.options.redirect, "error");
await api.apiRequest("/api/setup", { method: "PUT", operationId: "operation", body: { max_tokens: "18446744073709551615" } }, { csrfToken: "test-csrf" }, fetcher);
assert.equal(captured.options.headers["X-CSRF-Token"], "test-csrf");
assert.equal(captured.options.headers["Idempotency-Key"], "operation");
assert.equal(JSON.parse(captured.options.body).max_tokens, "18446744073709551615");
await assert.rejects(api.apiRequest("/api/setup", { method: "PUT" }, {}, fetcher));
await assert.rejects(api.apiRequest("/api/status", {}, {}, async () => ({ ok: false, status: 403 })));
await assert.rejects(api.apiRequest("/api/status", {}, {}, async () => ({ ok: true, headers: { get: () => "text/html" } })));
await assert.rejects(api.apiRequest("/api/status", {}, {}, async () => ({ ok: true, headers: { get: () => "application/json" }, json: async () => ({ schema_version: 2 }) })));
await assert.rejects(api.apiRequest("/api/status", {}, {}, async () => ({ ok: true, headers: { get: () => "application/json" }, json: async () => { throw new SyntaxError("private response content"); } })), /Local service returned invalid JSON/);
console.log("PASS: integer budget precision, measured/null semantics, authenticated request envelope and API error handling");
