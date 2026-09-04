# LYCAN OS 2.0

LYCAN is a pure Electron guest operating environment for Windows. It runs as a normal Electron application and keeps the guest filesystem, memory model, process table, package registry, network state, and snapshots inside `%LOCALAPPDATA%\LYCAN`.

## What changed in 2.0

The native C++ helper and Windows installer pipeline are no longer part of the running OS. The guest runtime is implemented directly in JavaScript inside Electron. The UI, guest VM/runtime, package system, snapshots, diagnostics, settings, Crawford boundary surface, and Gecko bridge remain part of the application.

This is a **real application-level virtual environment**, not a hardware hypervisor. It does not pretend to boot a second kernel. The guest state is actually persisted to disk, its filesystem is isolated beneath the LYCAN data directory, its virtual memory pages are allocated by the runtime, processes have real lifecycle state, snapshots serialize and restore guest state, and `.lypkg` packages are validated and installed into the guest registry.

## Run it

From Windows, double-click:

```text
run-lycan.bat
```

The first run installs Electron dependencies with npm, validates the source, and launches the Electron shell. Future runs launch directly.

You can also run manually:

```powershell
cd frontend
npm install
npm run check
npm start
```

## Safety boundary

LYCAN is a normal Windows application. It does not replace Windows, alter boot configuration, repartition disks, or install another operating system. Guest data is kept under `%LOCALAPPDATA%\LYCAN`.

## Core guest features

- Radial LYCAN desktop with floating applications
- ARES JavaScript guest runtime
- Real persistent LYFS filesystem
- Virtual 4 KiB page allocator and guest process table
- Guest-only VNET0 network state
- Snapshots with file/state serialization and restore
- Diagnostics and runtime telemetry
- `.lypkg` installation with manifest + SHA-256 validation
- Sandboxed Electron package applications with permission gates
- Gecko bridge for an installed Firefox executable
- Crawford isolation/control-plane surface
- Persistent interface settings

The old `src/` native VM code is retained as historical/reference material, but the Electron app no longer depends on it.
