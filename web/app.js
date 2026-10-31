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
  if (value === null || value === undefined) return "Unavailable";
  const amount = BigInt(integerValue(value));
  const fraction = (amount % 1000000000n).toString().padStart(9, "0").replace(/0+$/, "");
  return `${amount / 1000000000n}${fraction ? `.${fraction}` : ""}`;
}

export function measuredValue(value, type = "number") {
  if (value === null || value === undefined) return "Unavailable";
  if (!["string", "number"].includes(typeof value) || String(value).trim() === "") return "Unavailable";
  try {
    if (type === "integer") return BigInt(integerValue(value)).toLocaleString();
    if (type === "cost") return `$${nanosToDollars(value)}`;
    const number = Number(value);
    if (!Number.isFinite(number) || number < 0 || typeof value === "boolean") return "Unavailable";
    if (type === "percent") return number <= 1 ? `${(number * 100).toFixed(1)}%` : "Unavailable";
    return `${number.toLocaleString(undefined, { maximumFractionDigits: 3 })}${type === "ms" ? " ms" : ""}`;
  } catch {
    return "Unavailable";
  }
}

export async function apiRequest(path, options = {}, session = {}, fetcher = fetch) {
  const controller = new AbortController();
  const timer = setTimeout(() => controller.abort(), 8000);
  try {
    const method = options.method || "GET";
    const headers = { Accept: "application/json" };
    if (method !== "GET") {
      if (!session.csrfToken || !options.operationId) throw new Error("Authenticated operation context is required.");
      headers["Content-Type"] = "application/json";
      headers["X-CSRF-Token"] = session.csrfToken;
      headers["Idempotency-Key"] = options.operationId;
    }
    const response = await fetcher(path, {
      method, headers, credentials: "same-origin", cache: "no-store", redirect: "error",
      signal: controller.signal, body: options.body ? JSON.stringify(options.body) : undefined,
    });
    if (!response.ok) throw new Error(`Local service returned HTTP ${response.status}.`);
    if (!(response.headers.get("content-type") || "").includes("application/json")) throw new Error("The local service must return JSON.");
    const data = await response.json();
    if (!data || data.schema_version !== 1) throw new Error("Unsupported service response version.");
    return data;
  } catch (error) {
    if (error.name === "AbortError") throw new Error("Service request timed out. Reconcile a submitted operation before retrying.");
    if (error instanceof SyntaxError) throw new Error("Local service returned invalid JSON.");
    if (error instanceof TypeError) throw new Error("Local benchmark service unavailable. Start it, then refresh.");
    throw error;
  } finally {
    clearTimeout(timer);
  }
}

if (typeof document !== "undefined") {
  const byId = (id) => document.getElementById(id);
  const state = { status: null, setup: null, runs: [], selectedRunId: "", busy: false, pendingOperationId: null, refreshing: false };
  const metricDefinitions = [
    ["answer_accuracy", "Graded answer accuracy", "percent"],
    ["source_recall", "Source recall · declared metric", "percent"],
    ["retrieval_p50_ms", "Retrieval · median", "ms"],
    ["retrieval_p95_ms", "Retrieval · p95", "ms"],
    ["answer_p50_ms", "Complete answer · median", "ms"],
    ["billable_tokens", "Billable tokens", "integer"],
    ["cpu_delta_ms", "Local process CPU", "ms"],
    ["resident_bytes", "Sampled resident bytes", "integer"],
    ["lifetime_peak_resident_bytes", "Process lifetime peak bytes", "integer"],
    ["cost_nanos", "Accounted cost · USD", "cost"],
  ];

  function notice(message, error = false) {
    byId("notice").textContent = message;
    byId("notice").classList.toggle("error", error);
  }

  function renderMetrics(metrics = {}) {
    byId("metrics").replaceChildren(...metricDefinitions.map(([key, label, type]) => {
      const item = document.createElement("dl");
      item.className = "metric";
      const term = document.createElement("dt");
      term.textContent = label;
      const value = document.createElement("dd");
      value.textContent = measuredValue(metrics[key], type);
      value.classList.toggle("unavailable", value.textContent === "Unavailable");
      item.append(term, value);
      return item;
    }));
  }

  function updateControls() {
    const authenticated = state.status?.authenticated === true;
    const available = authenticated && !state.busy && !state.pendingOperationId;
    byId("save-setup").disabled = !available || state.status?.capabilities?.can_configure !== true;
    byId("start-run").disabled = !available || !state.setup?.revision || state.status?.capabilities?.can_start !== true;
    const run = state.runs.find((item) => item.id === state.selectedRunId);
    byId("resume-run").disabled = !available || run?.can_resume !== true;
    byId("stop-run").disabled = !available || run?.can_stop !== true;
    byId("run-status").textContent = run?.status || "Unavailable";
    byId("run-id").textContent = run?.id || "Unavailable";
    byId("run-config").textContent = run?.configuration_hash || "Unavailable";
    byId("run-progress").textContent = run?.completed !== undefined && run?.expected !== undefined
      ? `${measuredValue(run.completed, "integer")} / ${measuredValue(run.expected, "integer")}` : "Unavailable";
    byId("setup-state").textContent = state.setup?.revision ? "Saved setup" : "Setup unavailable";
  }

  function applySetup(setup) {
    state.setup = setup;
    const mapping = {
      provider: "provider", endpoint_url: "endpoint", reader_model_id: "reader-id", reader_model_version: "reader-version",
      judge_model_id: "judge-id", judge_model_version: "judge-version", max_requests: "max-requests", max_tokens: "max-tokens", seed: "seed",
    };
    for (const [field, id] of Object.entries(mapping)) byId(id).value = setup?.[field] ?? "";
    byId("max-cost").value = setup?.max_cost_nanos !== undefined ? nanosToDollars(setup.max_cost_nanos) : "";
    byId("setup-revision").textContent = setup?.revision ? `Saved revision: ${setup.revision}` : "Saved revision unavailable";
    updateControls();
  }

  function renderRuns() {
    const select = byId("run-select");
    const first = document.createElement("option");
    first.value = "";
    first.textContent = "Select a run";
    select.replaceChildren(first, ...state.runs.map((run) => {
      const option = document.createElement("option");
      option.value = run.id;
      option.textContent = `${run.id} · ${run.status}`;
      return option;
    }));
    if (!state.runs.some((run) => run.id === state.selectedRunId)) state.selectedRunId = "";
    select.value = state.selectedRunId;
    updateControls();
  }

  async function loadResults() {
    renderMetrics();
    byId("case-results").replaceChildren();
    if (!state.selectedRunId) {
      byId("result-context").textContent = "Select a run on Control to load its results.";
      byId("coverage").textContent = "Coverage unavailable";
      return;
    }
    const id = state.selectedRunId;
    const data = await apiRequest(`/api/runs/${encodeURIComponent(id)}/results`);
    if (id !== state.selectedRunId) return;
    renderMetrics(data.metrics);
    byId("result-context").textContent = `${id} · ${data.measurement_scope || "Measurement scope unavailable"}`;
    byId("coverage").textContent = data.summary
      ? `${measuredValue(data.summary.graded, "integer")} graded / ${measuredValue(data.summary.expected, "integer")} expected` : "Coverage unavailable";
    for (const record of data.cases || []) {
      const row = document.createElement("tr");
      for (const value of [record.case_id, record.status, record.correct === true ? "Correct" : record.correct === false ? "Incorrect" : "Unavailable", Array.isArray(record.source_ids) ? record.source_ids.join(", ") : "Unavailable"]) {
        const cell = document.createElement("td");
        cell.textContent = value ?? "Unavailable";
        row.append(cell);
      }
      byId("case-results").append(row);
    }
  }

  async function refresh() {
    if (state.refreshing || state.busy) return;
    state.refreshing = true;
    byId("refresh").disabled = true;
    try {
      const status = await apiRequest("/api/status");
      state.status = status;
      byId("connection-state").textContent = status.authenticated ? "Connected" : "Sign-in required";
      byId("connection-state").classList.toggle("connected", status.authenticated === true);
      if (state.pendingOperationId && ["accepted", "rejected", "completed"].includes(status.operation_states?.[state.pendingOperationId])) state.pendingOperationId = null;
      if (status.authenticated) {
        const [setupData, runsData] = await Promise.all([apiRequest("/api/setup"), apiRequest("/api/runs")]);
        state.runs = Array.isArray(runsData.runs) ? runsData.runs : [];
        if (document.activeElement?.closest("#setup-form") === null) applySetup(setupData.setup);
        else state.setup = setupData.setup;
        renderRuns();
        if (location.hash === "#results") await loadResults();
      } else {
        state.setup = null;
        state.runs = [];
        renderRuns();
      }
      notice(state.pendingOperationId ? `Operation ${state.pendingOperationId} needs service reconciliation before another action.` : status.authenticated ? "Connected to the local service. Review the saved setup and run limits." : "Sign in through the local benchmark service, then refresh.");
    } catch (error) {
      state.status = null;
      byId("connection-state").textContent = "Disconnected";
      byId("connection-state").classList.remove("connected");
      notice(error.message, true);
    } finally {
      state.refreshing = false;
      byId("refresh").disabled = false;
      updateControls();
    }
  }

  async function mutate(path, method, body) {
    if (state.busy || state.pendingOperationId || !state.status?.authenticated || !state.status.csrf_token) throw new Error("An authenticated, reconciled service session is required.");
    if (typeof globalThis.crypto?.randomUUID !== "function") throw new Error("Open the authenticated dashboard over HTTPS or localhost.");
    const operationId = crypto.randomUUID();
    state.busy = true;
    state.pendingOperationId = operationId;
    updateControls();
    try {
      const data = await apiRequest(path, { method, body, operationId: state.pendingOperationId }, { csrfToken: state.status.csrf_token });
      state.pendingOperationId = null;
      return data;
    } catch (error) {
      throw new Error(`${error.message} Refresh to reconcile operation ${state.pendingOperationId}.`);
    } finally {
      state.busy = false;
      updateControls();
    }
  }

  function navigate() {
    const page = ["control", "setup", "results", "credits"].includes(location.hash.slice(1)) ? location.hash.slice(1) : "control";
    for (const section of document.querySelectorAll(".page")) section.hidden = section.id !== page;
    for (const link of document.querySelectorAll("nav a")) {
      if (link.hash === `#${page}`) link.setAttribute("aria-current", "page");
      else link.removeAttribute("aria-current");
    }
    if (page === "results" && state.status?.authenticated) loadResults().catch((error) => notice(error.message, true));
  }

  byId("setup-form").addEventListener("submit", async (event) => {
    event.preventDefault();
    try {
      const endpoint = new URL(byId("endpoint").value);
      if (!["https:", "http:"].includes(endpoint.protocol) || endpoint.username || endpoint.password || endpoint.search || endpoint.hash) throw new Error("Use an HTTP(S) endpoint without credentials, query parameters or fragments.");
      const body = { schema_version: 1, provider: byId("provider").value.trim(), endpoint_url: endpoint.href,
        reader_model_id: byId("reader-id").value.trim(), reader_model_version: byId("reader-version").value.trim(),
        judge_model_id: byId("judge-id").value.trim(), judge_model_version: byId("judge-version").value.trim(),
        max_requests: integerValue(byId("max-requests").value), max_tokens: integerValue(byId("max-tokens").value),
        max_cost_nanos: dollarsToNanos(byId("max-cost").value), seed: integerValue(byId("seed").value) };
      const data = await mutate("/api/setup", "PUT", body);
      applySetup(data.setup);
      notice("Setup saved. Review the selected cases on Control.");
    } catch (error) { notice(error.message, true); }
  });

  byId("run-form").addEventListener("submit", async (event) => {
    event.preventDefault();
    try {
      const caseIds = byId("case-ids").value.split(/\r?\n/).map((id) => id.trim()).filter(Boolean);
      if (!caseIds.length || new Set(caseIds).size !== caseIds.length) throw new Error("Select at least one unique case ID.");
      const data = await mutate("/api/runs", "POST", { schema_version: 1, setup_revision: state.setup?.revision,
        dataset: byId("dataset").value, condition: byId("condition").value, case_ids: caseIds });
      state.selectedRunId = data.run.id;
      await refresh();
      notice("Run accepted by the service. Its recorded status appears on Control.");
    } catch (error) { notice(error.message, true); }
  });

  for (const action of ["resume", "stop"]) byId(`${action}-run`).addEventListener("click", async () => {
    try {
      await mutate(`/api/runs/${encodeURIComponent(state.selectedRunId)}/${action}`, "POST", { schema_version: 1 });
      await refresh();
      notice(`${action === "stop" ? "Stop" : "Resume"} request accepted. Status reflects the service response.`);
    } catch (error) { notice(error.message, true); }
  });
  byId("run-select").addEventListener("change", () => { state.selectedRunId = byId("run-select").value; updateControls(); });
  byId("refresh").addEventListener("click", refresh);
  window.addEventListener("hashchange", navigate);
  window.addEventListener("visibilitychange", () => { if (!document.hidden) refresh(); });
  setInterval(() => {
    if (!document.hidden && state.status?.authenticated && state.runs.some((run) => ["queued", "running", "stop_requested"].includes(run.status))) refresh();
  }, 5000);
  renderMetrics();
  navigate();
  refresh();
}
