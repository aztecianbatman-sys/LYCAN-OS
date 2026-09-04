# LYPKG SDK

LYPKG is LYCAN's application package format. The SDK is intentionally small: an app is a web application contained in `app/`, described by `manifest.json`, and protected by `checksums.sha256`.

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
  "permissions": ["storage"]
}
```

`entry` and `icon` must remain inside `app/`. When `entry` is omitted, the runtime uses `app/index.html`.

Supported permission labels currently include:

- `storage` — guest-local application storage
- `network` — guest network access
- `notifications` — LYCAN notification surface

A permission declaration does not grant host access. LYCAN applications remain guest applications and cannot directly access the Windows filesystem, host processes, boot configuration, or partition layout.

## Create a project

```powershell
.\tools\new-lypkg.ps1 -Id my-app -Name "My App"
```

This creates a ready-to-edit application skeleton with an HTML entrypoint and icon.

## Package

```powershell
.\tools\make-lypkg.ps1 -AppDirectory .\my-app -Output .\dist\my-app-1.0.0.lypkg
```

The packager regenerates SHA-256 checksums and validates the ID, version, entrypoint, icon, and permission declarations before producing the archive.

## Install

Install through **LYCAN Store → Import .LYPKG**. Installation is explicit; downloaded packages are not executed automatically.

## Design rules

Keep application state under the guest application directory. Avoid assuming Windows paths. Treat the browser-like surface as a web UI, not as a way to escape LYCAN's guest boundary.
