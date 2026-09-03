# LYCAN OS — Enterprise Production Roadmap

LYCAN is a native Windows-hosted virtual operating environment. The target is production-grade engineering, not a visual mock-up.

## Engineering pillars

1. **ARES-CPU** — deterministic execution, complete decoding/execution, interrupts, exceptions, tracing, debugger hooks and assembler validation.
2. **Memory** — protected mappings, permissions, page-fault model, allocation diagnostics, kernel/user separation and resource accounting.
3. **Storage** — virtual disks, partitions, formatting, snapshots, integrity checks, transactional LYFS, journaling and crash recovery.
4. **Kernel** — processes, threads, scheduler, contexts, handles, IPC, synchronization, syscalls, device manager, services, time and logging.
5. **Desktop** — native Win32 shell, windows, dialogs, workspaces, notifications, clipboard, drag/drop, DPI scaling, accessibility and recovery.
6. **Apps + SDK** — native system apps, stable versioned Lycan API/ABI, capability checks, async APIs, templates, debugger and documentation.
7. **Packages** — `.lypkg`, dependency solving, signatures, publisher trust, revocation, SBOM/provenance, updates and rollback.
8. **Networking** — TCP/UDP/DNS/HTTP/HTTPS/TLS/WebSocket abstractions, permissions, diagnostics and offline behavior.
9. **Lycan Web** — real Mozilla Gecko embedding; no Chromium disguise, iframe substitute or Firefox-process shortcut.
10. **Crawford** — supplied source remains unchanged; LYCAN provides a permissioned native integration boundary.
11. **Security** — capability security, sandboxing, isolation, signed packages, secure defaults, audit logging and threat-model testing.
12. **Operations** — structured logs, health checks, diagnostics bundles, opt-in telemetry, support mode, policies and admin/user roles.
13. **Updates** — signed release channels, atomic update, rollback, recovery and release manifests/checksums.
14. **Quality** — unit/integration/regression tests, sanitizers/static analysis, benchmarks, deterministic CI and Windows installer smoke tests.

## Release gates

A feature is not considered complete because its UI exists. Each subsystem needs an implementation, tests, error handling, documentation and a CI path. External dependencies must be identified explicitly.

The Windows release gate is:

`configure -> build -> test -> package -> installer -> smoke test -> checksum/provenance`

The repository should never claim an EXE is production-ready until that pipeline succeeds.
