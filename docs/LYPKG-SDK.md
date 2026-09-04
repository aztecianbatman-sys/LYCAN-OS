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
  "permissions": ["storage"]
}
```

`entry` and `icon` must remain inside `app/`. When `entry` is omitted, the runtime uses `app/index.html`.

## Runtime permissions

Permissions are enforced by the LYCAN package runtime. Declaring a permission is not the same as receiving Windows access.

- `storage` — creates a private application data directory below the LYCAN package registry.
- `network` — permits HTTP/HTTPS requests from the package's isolated session.
- `external` — permits an app to request opening an HTTP/HTTPS URL in the user's external browser.
- `clipboard-read` — reserved for the controlled clipboard API.
- `clipboard-write` — reserved for the controlled clipboard API.

Packages without `network` cannot make HTTP/HTTPS requests through their isolated Electron session. Packages without `external` cannot launch external browser URLs through the package bridge.

Every package runs with `contextIsolation`, `nodeIntegration: false`, Chromium sandboxing, and a dedicated persistent session partition. Package applications cannot directly access the Windows filesystem, host processes, boot configuration, or partition layout.

## Create a project

```powershell
.\tools\new-lypkg.ps1 -Id my-app -Name "My App"
```

This creates a ready-to-edit application skeleton with an HTML entrypoint and icon.

## Package

```powershell
.\tools\make-lypkg.ps1 -AppDirectory .\my-app -Output .\dist\my-app-1.0.0.lypkg
```

The packager regenerates SHA-256 checksums and validates the manifest before producing the archive.

## Install

Install through **LYCAN Store → Import .LYPKG**. Installation is explicit; downloaded packages are not executed automatically.

## Design rules

Keep application state in the package's private guest storage area. Avoid Windows paths. Request the smallest permission set necessary for the application. Treat the package surface as a web application, not as a mechanism for escaping LYCAN's guest boundary.
