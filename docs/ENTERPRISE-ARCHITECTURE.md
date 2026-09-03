# LYCAN OS — Enterprise Architecture

## Product contract

LYCAN OS is a native Windows-hosted virtual operating environment. Windows remains the host OS. LYCAN provides its own virtual CPU, memory, storage, kernel model, filesystem, devices, native shell, applications, package system, networking boundary, browser integration boundary, security model, diagnostics and update architecture.

The project uses a strict rule: advertised functionality must be implemented and tested, or explicitly identified as an external dependency/adapter.

## Engineering layers

1. **Host layer** — Win32 process, display/input integration, timing, filesystem bridge and crash containment.
2. **Virtual hardware** — ARES-CPU, virtual RAM/MMU, virtual disk, framebuffer, timer, keyboard, mouse and network device boundary.
3. **Firmware/boot** — firmware ROM, bootloader, kernel entry and service startup.
4. **Kernel** — processes, threads, scheduler, memory manager, syscalls, IPC, device manager, permissions, filesystem and service manager.
5. **Storage** — LYFS metadata, allocation, persistence, transactions, journal/recovery, snapshots and integrity verification.
6. **Security** — identities, capabilities, process isolation, package signatures, trust policy, secrets boundary and audit events.
7. **Native shell** — windows, workspaces, dialogs, notifications, clipboard, drag/drop, accessibility, DPI and keyboard navigation.
8. **System applications** — terminal, files, settings, monitor, process manager, logs, disk utility, editor, calculator, updater, package manager and browser shell.
9. **Developer platform** — stable Lycan API/ABI, SDK, package format, debugger, templates and diagnostics.
10. **Connectivity** — guest networking abstractions, permissions, diagnostics and browser networking.
11. **AI boundary** — Crawford integration through explicit OS capabilities; no raw kernel access.
12. **Release engineering** — reproducible builds, tests, signing/provenance, installer generation, smoke tests and rollback.

## Enterprise quality gates

Every subsystem should have:

- Unit tests for deterministic logic.
- Integration tests for subsystem boundaries.
- Failure-path tests and recovery tests.
- Input validation and bounds checks.
- Structured diagnostics.
- Stable error codes where externally visible.
- Documentation of security assumptions.
- Performance benchmarks for hot paths.
- Windows CI coverage where host behavior is involved.
- No placeholder implementations hidden behind production feature names.

## Reliability goals

LYCAN should degrade safely. A failed guest process must not crash the host. A corrupt guest file must not become arbitrary host access. A package failure must be reversible. An interrupted update must leave a bootable previous version. Diagnostics must remain useful when a subsystem fails.

## Security goals

The security model is capability-oriented. Guest applications receive only the permissions they need. Host-facing operations are mediated by explicit adapters. Package provenance and signatures are verified before trust is granted. Secrets are never embedded in source code or release artifacts.

## Release tiers

- **Nightly:** experimental, CI-tested, not production supported.
- **Preview:** feature-complete candidate with known limitations documented.
- **Stable:** Windows installer, smoke-tested, signed release metadata, rollback path and published checksums.
- **LTS:** frozen compatibility baseline with security and critical fixes only.

## Non-goals

LYCAN is not a replacement Windows kernel, not a claim of hardware-level isolation from Windows, and not a fake browser implementation. External engines such as Gecko remain explicit dependencies until a real embedding path is integrated and tested.
