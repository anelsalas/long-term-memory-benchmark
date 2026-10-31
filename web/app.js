const U64_MAX = 18446744073709551615n;

export function integerValue(value) {
  if (typeof value === "number" && !Number.isSafeInteger(value)) throw new Error("Use decimal text for large integers.");
  if (!/^\d+$/.test(String(value))) throw new Error("Enter a whole nonnegative number.");
  const result = BigInt(value);
  if (result > U64_MAX) throw new Error("The number exceeds the supported range.");
  return result.toString();
}

export function dollarsToNanos(value) {
  const match = /^(\d+)(?:\.(\d{1,9}))?$/.exec(String(value));
  if (!match) throw new Error("Enter a USD amount with up to nine decimal places.");
  return integerValue((BigInt(match[1]) * 1000000000n + BigInt((match[2] || "").padEnd(9, "0"))).toString());
}

export function nanosToDollars(value) {
  const amount = BigInt(integerValue(value));
  const fraction = (amount % 1000000000n).toString().padStart(9, "0").replace(/0+$/, "");
  return `${amount / 1000000000n}${fraction ? `.${fraction}` : ""}`;
}

// Shows JSON output indented; any other text is shown as printed.
export function formatOutput(text) {
  try {
    return JSON.stringify(JSON.parse(text), null, 2);
  } catch {
    return text;
  }
}

// Calls the dashboard service. Changes are sent as JSON; errors carry the service's message.
export async function api(path, { method = "GET", body } = {}, fetcher = (...args) => globalThis.fetch(...args)) {
  const options = { method, headers: { Accept: "application/json" }, cache: "no-store", credentials: "same-origin" };
  if (body !== undefined) {
    options.headers["Content-Type"] = "application/json";
    options.body = JSON.stringify(body);
  }
  let response;
  try {
    response = await fetcher(path, options);
  } catch {
    throw new Error("Dashboard service unavailable. Start it, then refresh.");
  }
  const json = (response.headers.get("content-type") || "").startsWith("application/json");
  const data = json ? await response.json() : await response.text();
  if (!response.ok) throw new Error((json && data?.error) || `Service returned HTTP ${response.status}.`);
  return data;
}

if (typeof document !== "undefined") {
  const byId = (id) => document.getElementById(id);
  const state = { connected: false, settings: null, runs: [], selected: "", busy: false };
  const modelText = (m) => (m ? `${m.provider} · ${m.id} · ${m.version}` : "Unavailable");

  function notice(message, error = false) {
    byId("notice").textContent = message;
    byId("notice").classList.toggle("error", error);
  }

  function render() {
    const run = state.runs.find((r) => r.id === state.selected);
    const active = state.runs.some((r) => r.status === "running");
    byId("connection-state").textContent = state.connected ? "Connected" : "Disconnected";
    byId("connection-state").classList.toggle("connected", state.connected);
    byId("save-setup").disabled = !state.connected || state.busy;
    byId("start-run").disabled = !state.connected || state.busy || !state.settings || active;
    byId("stop-run").disabled = !state.connected || state.busy || run?.status !== "running";
    byId("setup-state").textContent = state.settings ? "Saved setup" : "Setup unavailable";
    byId("summary-reader").textContent = modelText(state.settings?.reader);
    byId("summary-judge").textContent = modelText(state.settings?.judge);
    byId("summary-limits").textContent = state.settings
      ? `${state.settings.max_requests} requests · ${state.settings.max_tokens} tokens · $${nanosToDollars(state.settings.max_cost_nanos)}`
      : "Unavailable";
    byId("run-status").textContent = run?.status || "Unavailable";
    byId("run-id").textContent = run?.id || "Unavailable";
    byId("run-exit").textContent = run?.exit_code ?? "Unavailable";
  }

  function renderRuns() {
    const select = byId("run-select");
    if (!state.runs.some((r) => r.id === state.selected)) state.selected = state.runs.at(-1)?.id || "";
    const options = [...state.runs].reverse().map((run) => {
      const option = document.createElement("option");
      option.value = run.id;
      option.textContent = `${run.id} · ${run.status}`;
      return option;
    });
    if (!options.length) {
      const empty = document.createElement("option");
      empty.value = "";
      empty.textContent = "No runs yet";
      options.push(empty);
    }
    select.replaceChildren(...options);
    select.value = state.selected;
  }

  function fillSetup(settings) {
    const fields = {
      endpoint: settings?.endpoint_url, "reader-provider": settings?.reader.provider, "reader-id": settings?.reader.id,
      "reader-version": settings?.reader.version, "judge-provider": settings?.judge.provider, "judge-id": settings?.judge.id,
      "judge-version": settings?.judge.version, "max-requests": settings?.max_requests, "max-tokens": settings?.max_tokens,
      "max-cost": settings ? nanosToDollars(settings.max_cost_nanos) : "", seed: settings?.seed,
    };
    for (const [id, value] of Object.entries(fields)) byId(id).value = value ?? "";
  }

  async function loadOutput() {
    if (!state.selected) {
      byId("output-context").textContent = "Select a run on Control to see its output.";
      byId("output-text").textContent = "No output yet.";
      return;
    }
    const id = state.selected;
    const text = await api(`/api/runs/${encodeURIComponent(id)}/output`);
    if (id !== state.selected) return;
    const run = state.runs.find((r) => r.id === id);
    byId("output-context").textContent = `${id} · ${run?.status || "unknown"}`;
    byId("output-text").textContent = formatOutput(text) || "No output yet.";
  }

  async function refresh(quiet = false) {
    try {
      const [settingsData, runsData] = await Promise.all([api("/api/settings"), api("/api/runs")]);
      state.connected = true;
      state.settings = settingsData.settings;
      state.runs = runsData.runs;
      if (!byId("setup-form").contains(document.activeElement)) fillSetup(state.settings);
      renderRuns();
      if (location.hash === "#output") await loadOutput();
      if (!quiet) notice(state.settings ? "Connected. Review the saved setup, then start a run." : "Connected. Save a setup before starting a run.");
    } catch (error) {
      state.connected = false;
      notice(error.message, true);
    } finally {
      render();
    }
  }

  async function act(action, success) {
    state.busy = true;
    render();
    try {
      await action();
      notice(success);
    } catch (error) {
      notice(error.message, true);
    } finally {
      state.busy = false;
      await refresh(true);
    }
  }

  function navigate() {
    const pages = ["control", "setup", "output", "credits"];
    const page = pages.includes(location.hash.slice(1)) ? location.hash.slice(1) : "control";
    for (const section of document.querySelectorAll(".page")) section.hidden = section.id !== page;
    for (const link of document.querySelectorAll("nav a")) {
      if (link.hash === `#${page}`) link.setAttribute("aria-current", "page");
      else link.removeAttribute("aria-current");
    }
    if (page === "output" && state.connected) loadOutput().catch((error) => notice(error.message, true));
  }

  byId("setup-form").addEventListener("submit", (event) => {
    event.preventDefault();
    act(async () => {
      const text = (id) => byId(id).value.trim();
      const data = await api("/api/settings", { method: "PUT", body: {
        schema_version: 1, endpoint_url: text("endpoint"),
        reader: { provider: text("reader-provider"), id: text("reader-id"), version: text("reader-version") },
        judge: { provider: text("judge-provider"), id: text("judge-id"), version: text("judge-version") },
        max_requests: integerValue(text("max-requests")), max_tokens: integerValue(text("max-tokens")),
        max_cost_nanos: dollarsToNanos(text("max-cost")), seed: integerValue(text("seed")),
      } });
      fillSetup(data.settings);
    }, "Setup saved.");
  });
  byId("start-run").addEventListener("click", () => act(async () => {
    const data = await api("/api/runs", { method: "POST", body: {} });
    state.selected = data.run.id;
  }, "Run started."));
  byId("stop-run").addEventListener("click", () => act(
    () => api(`/api/runs/${encodeURIComponent(state.selected)}/stop`, { method: "POST", body: {} }), "Stop requested."));
  byId("run-select").addEventListener("change", () => {
    state.selected = byId("run-select").value;
    render();
  });
  byId("refresh").addEventListener("click", () => refresh());
  window.addEventListener("hashchange", navigate);
  window.addEventListener("visibilitychange", () => { if (!document.hidden) refresh(true); });
  setInterval(() => {
    if (!document.hidden && state.connected && state.runs.some((r) => r.status === "running")) refresh(true);
  }, 3000);
  navigate();
  refresh();
}
