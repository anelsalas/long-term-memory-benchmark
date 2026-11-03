# Mem0 adapter dependencies

This adapter uses the open-source package [Mem0](https://github.com/mem0ai/mem0)
(`mem0ai` 3.3.1) and `better-sqlite3` 12.6.2, pinned in the lockfile.
Run `npm ci` to install and `npm test` to check local SQLite vector persistence.
The storage check uses fictional data and turns Mem0 telemetry off before loading
the package. The benchmark integration is under development.
