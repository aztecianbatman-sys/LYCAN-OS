# LYCAN Store + Lycan Web

LYCAN 0.5.0 introduces the foundation for two user-facing services.

## Lycan Store

The Store is intentionally free: LYCAN does not charge users for packages. The catalog lives in `store/catalog.json` and describes package identity, version, publisher, download URL and integrity metadata.

Packages are downloaded through the host's WinHTTP boundary and must remain inside the Lycan package boundary. A future package manager will add:

- `.lypkg` package parsing
- SHA-256 verification
- publisher signatures and trust roots
- dependency solving
- install/uninstall/upgrade/rollback
- permissions/capabilities
- per-package sandboxing
- provenance/SBOM metadata

A package must never receive unrestricted host access simply because it came from the Store.

## Lycan Web

Lycan Web is designed around Mozilla Gecko, not Chromium. `GeckoRuntime` currently discovers a Gecko/Firefox installation and exposes an explicit runtime boundary. Actual Gecko embedding remains an external dependency until the embedding SDK/runtime is supplied and integrated.

The project deliberately does **not** claim that detecting Firefox is equivalent to embedding Gecko. The next implementation stage is a real Gecko embedding adapter with navigation, tabs, downloads, storage isolation and capability mediation.

## Version

This work moves the project baseline from 0.4.x to 0.5.0 while preserving the existing Windows-hosted virtual-OS model: Windows remains the host and LYCAN runs as a normal application.
