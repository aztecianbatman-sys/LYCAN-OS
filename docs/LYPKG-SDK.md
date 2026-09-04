# LYPKG SDK

LYPKG is LYCAN's application package format. An app is a web application contained in `app/`, described by `manifest.json`, and protected by `checksums.sha256`.

## Project layout

```text
my-app/
├── manifest.json
└── app/
    ├── index.html
    ├── app.js
    └── assets/
        └── icon.svg
```

## Manifest

```json
{
  "format": 1,
  "type": "lycan-app",
  "id": "my-app",
  "name": "My App",
  "version": "1.0.0",
  "description": "Example LYCAN application.",
  "entry": "app/index.html",
  "icon": "app/assets/icon.svg",
  "permissions": ["storage", "network", "notifications"],
  "storageQuotaMB": 32
}
```

`entry` and `icon` must remain inside `app/`. When `entry` is omitted, the runtime uses `app/index.html`. `storageQuotaMB` is the application's guest-local quota and must be between 1 and 1024 MB.

## Permissions

Supported permission labels are `storage`, `network`, `external`, `notifications`, `clipboard-read`, and `clipboard-write`.

A permission never grants Windows access. LYCAN apps run with Chromium sandboxing, context isolation, no Node integration, and a package-specific persistent session.

## Storage API

The package preload exposes a scoped `window.lycanApp.storage` API. All paths are relative to the application's guest storage and may use the `data`, `config`, or `cache` bucket.

```js
await lycanApp.storage.write('data', 'notes.txt', 'hello');
const text = await lycanApp.storage.read('data', 'notes.txt');
const entries = await lycanApp.storage.list('data', '');
await lycanApp.storage.delete('cache', 'old.tmp');
await lycanApp.storage.usage();
await lycanApp.storage.quota();
```

Storage is persisted under the LYCAN guest data root and survives application and operating-system restarts. Quota is enforced by the ARES runtime before a write is committed.

## Runtime APIs

`lycanApp.network.status()` reads the guest VNET state when `network` permission is present. `lycanApp.requestExternal(url)` requires `external`. `lycanApp.notify(title, body)` requires `notifications`.

The lifecycle is managed by ARES: install/register, launch, suspend, resume, crash/error state, close, and uninstall. The Store uses the same registry and version information.

## Create a project

```powershell
.\tools\new-lypkg.ps1 -Id my-app -Name "My App"
```

## Package

```powershell
.\tools\make-lypkg.ps1 -AppDirectory .\my-app -Output .\dist\my-app-1.0.0.lypkg
```

## Install

Use **LYCAN Store → Import .LYPKG** or the repository download action. Installation validates the manifest and every SHA-256 entry before registering the package with ARES.

## Design rules

Keep application state inside `lycanApp.storage`. Never assume Windows paths. Do not use filesystem APIs from package code. Treat the package browser surface as a guest UI, not a route around the LYCAN boundary.
