# LYCAN OS

LYCAN is a Windows-hosted virtual operating environment. It does **not** replace, repartition, or boot over Windows. The Windows program hosts a self-contained LYCAN machine with its own virtual CPU, memory, filesystem, processes, packages, snapshots, desktop, and app model.

## v1.0 architecture

- ARES-CPU interpreter with deterministic virtual registers and instruction execution
- Virtual RAM/MMU with protected ranges
- LYFS virtual storage backed by a single host data directory
- Guest process manager and failure isolation
- Capability/security policy engine
- Snapshot save/restore
- `.lypkg` package manager with manifest, hashes, publisher trust and rollback metadata
- Native app host and built-in Terminal, Files, Store, Web, Settings and Diagnostics surfaces
- Polished Win32 desktop shell rendered by LYCAN itself
- HTTPS package download path with mandatory SHA-256 verification against the Store catalog
- Windows installer and CI release pipeline
- Gecko runtime discovery boundary for the real Mozilla engine integration

## Package security

Downloaded `.lypkg` archives are never installed merely because they can be opened. The package manager follows this order:

```text
DOWNLOAD
   ↓
READ PACKAGE
   ↓
CALCULATE SHA-256
   ↓
COMPARE WITH CATALOG
   ↓
MATCH?
 ┌─┴─┐
YES  NO
 ↓    ↓
INSTALL  REJECT
```

The catalog digest is mandatory and must be a valid 64-character SHA-256 value. A missing, malformed, or mismatching digest is a hard failure. Verification occurs **before** package extraction or guest filesystem creation.

A failed verification returns:

```text
LYCAN SECURITY

Package rejected.

Reason:
SHA-256 verification failed.

The package was NOT installed.
```

LYCAN does not report a package as verified unless the calculated archive digest actually matches the catalog digest.

## Safety boundary

LYCAN is an application and VM-like guest environment on top of Windows. It is experimental software, not a replacement Windows kernel or a security-certified sandbox. Package capability checks and process isolation are defense-in-depth features; they must not be treated as a hardened security boundary.

## Build

```text
cmake -S . -B build
cmake --build build --config Release
ctest --test-dir build -C Release --output-on-failure
```

On Windows, the GUI binary is `lycan-vm.exe`.

## Data

User data is stored under `%LOCALAPPDATA%\\LycanOS\\data` by the Windows shell. The VM never writes into Windows system directories as part of normal guest storage operations.

## Web / Gecko

The Web app is deliberately separated from Chromium/WebView2. The current runtime boundary discovers a Mozilla Gecko installation and reports its availability. Full in-process Gecko embedding requires shipping and integrating a licensed/redistributable Gecko embedding runtime and is kept as an explicit integration boundary rather than pretending that Firefox or Chromium is Gecko.
