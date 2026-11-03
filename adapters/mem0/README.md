# Mem0 adapter dependencies

This adapter uses the open-source package [Mem0](https://github.com/mem0ai/mem0)
(`mem0ai` 3.3.1) and `better-sqlite3` 12.6.2, pinned in the lockfile.
Run `npm ci` to install and `npm test` to check local SQLite vector persistence.
The offline tests use fictional data and local fake model endpoints. The adapter
turns Mem0 telemetry off before loading the package, requires local provider
endpoints, and uses `.db` filenames so Mem0's separate entity store gets its own
database. Mem0 3.3.1 uses additive extraction and hybrid search; report its
version when comparing it with earlier paper results. The benchmark HTTP
integration is under development.
