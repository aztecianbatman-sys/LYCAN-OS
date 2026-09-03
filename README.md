# LYCAN OS

LYCAN is a Windows-hosted virtual operating environment. It does **not** replace, repartition, or boot over Windows. The Windows program hosts a self-contained LYCAN machine with its own virtual CPU, memory, filesystem, processes, packages, snapshots, desktop, and app model.

## v1.0 architecture

- ARES-CPU interpreter with deterministic virtual registers and instruction execution
- Virtual RAM/MMU with protected ranges
- LYFS virtual storage backed by a single host data directory
- Guest process manager and failure isolation
- Capability/security policy engine
- Snapshot save/restore
- `.lypkg` package manager with manifest, hashes, publisher trust, installed-package database and transactional upgrades
- Native app host and built-in Terminal, Files, Store, Web, Settings and Diagnostics surfaces
- Polished Win32 desktop shell rendered by LYCAN itself
- HTTPS package download path with mandatory SHA-256 verification against the Store catalog
- Windows installer and CI release pipeline
- Gecko runtime discovery boundary for the real Mozilla engine integration

## Installed-package database

LYCAN maintains a persistent database at the guest path `/system/packages.db`. It is the source of truth for packages installed through the package manager rather than merely scanning `/apps` directories.

Each record stores:

```text
Package ID
Version
Publisher
Install location
Install time
Checksum (SHA-256)
Previous version
Permissions
Entry point
```

A record conceptually looks like:

```text
lycan-terminal
1.0.0
LYCAN
/apps/lycan-terminal
install-time: 1770000000
sha256:...
previous: (none)
permissions: lyfs.read,lyfs.write
```

The database is rewritten atomically at the package-manager transaction boundary and keeps one current record per package ID. Upgrades preserve the previous version in the `previousVersion` field, while the current archive checksum is recorded for integrity/audit purposes. Permissions come directly from the validated package manifest.

### Terminal `apps`

The Terminal `apps` command now reads the installed-package database and displays actual installed packages, including version, publisher, installation location and requested capabilities:

```text
ID                 VERSION    PUBLISHER    LOCATION                 PERMISSIONS
--------------------------------------------------------------------------------
lycan-terminal     1.0.0      LYCAN        /apps/lycan-terminal     lyfs.read, lyfs.write
```

This means an application cannot appear in `apps` simply because a random directory exists under `/apps`; it must have a package database record.

## Package security and transactional installation

Downloaded `.lypkg` archives are never installed merely because they can be opened. The package manager uses a transaction boundary:

```text
/package-cache/<id>.tmp
        ↓
   verify SHA-256
        ↓
      extract
        ↓
 validate staged manifest
        ↓
   check publisher
        ↓
 stage application
        ↓
 backup previous version
        ↓
      commit
        ↓
    INSTALLED
```

The live `/apps/<id>` directory is not modified while a package is being downloaded, verified, extracted, or validated. Extraction happens in an isolated temporary staging directory. Only after validation succeeds is the previous application moved to a same-filesystem backup and the staged directory promoted to `/apps/<id>`.

If a transaction fails before commit, staging is deleted and the previous version is untouched. If a failure occurs after the application swap, the package manager removes the new version and restores the backup. The package database is protected during the transaction as well, so a failed database update triggers filesystem rollback instead of leaving an unrecorded application behind.

The Store SHA-256 digest remains mandatory and is verified **before staging**. A missing, malformed, or mismatching digest is a hard security failure:

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
