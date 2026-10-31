import assert from "node:assert/strict";
import { readFile } from "node:fs/promises";

const source = await readFile(new URL("app.js", import.meta.url), "utf8");
const app = await import(`data:text/javascript;base64,${Buffer.from(source).toString("base64")}`);

assert.equal(app.integerValue("18446744073709551615"), "18446744073709551615");
assert.throws(() => app.integerValue("18446744073709551616"));
assert.throws(() => app.integerValue(-1));
assert.throws(() => app.integerValue(9007199254740992));
assert.equal(app.dollarsToNanos("0.000000001"), "1");
assert.equal(app.dollarsToNanos("12.345"), "12345000000");
assert.throws(() => app.dollarsToNanos("0.0000000001"));
assert.equal(app.nanosToDollars("12345000000"), "12.345");
assert.equal(app.nanosToDollars("0"), "0");

const reply = (status, type, body) => ({
  ok: status < 400, status, headers: { get: () => type },
  json: async () => body, text: async () => body,
});
let captured;
const fetcher = (status, type, body) => async (path, options) => { captured = { path, options }; return reply(status, type, body); };

assert.deepEqual(await app.api("/api/runs", {}, fetcher(200, "application/json", { runs: [] })), { runs: [] });
assert.equal(captured.options.method, "GET");
assert.equal(captured.options.headers["Content-Type"], undefined);
assert.equal(captured.options.cache, "no-store");

await app.api("/api/settings", { method: "PUT", body: { max_tokens: "18446744073709551615" } }, fetcher(200, "application/json", {}));
assert.equal(captured.options.headers["Content-Type"], "application/json");
assert.equal(JSON.parse(captured.options.body).max_tokens, "18446744073709551615");

assert.equal(await app.api("/api/runs/run-1/output", {}, fetcher(200, "text/plain; charset=utf-8", "line\n")), "line\n");
await assert.rejects(app.api("/api/runs", { method: "POST", body: {} },
  fetcher(409, "application/json", { error: "Save settings before starting a run" })), /Save settings/);
await assert.rejects(app.api("/x", {}, fetcher(500, "text/html", "<html>")), /HTTP 500/);
await assert.rejects(app.api("/x", {}, async () => { throw new TypeError("network"); }), /service unavailable/);
assert.equal(app.formatOutput('{"a":1}\n'), '{\n  "a": 1\n}');
assert.equal(app.formatOutput("plain line\n"), "plain line\n");
assert.equal(app.formatOutput(""), "");
console.log("PASS: uint64 and USD conversion, JSON requests, output formatting and service errors");
