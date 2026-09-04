# LYPKG 1.0

`.lypkg` is the native LYCAN application package format. A package is a ZIP archive whose extension is `.lypkg`.

```text
terminal-1.0.0.lypkg
├── manifest.json
├── app/
│   ├── app.html
│   ├── app.js
│   └── assets/
└── checksums.sha256
```

The manifest must identify an application and use `type: "lycan-app"`:

```json
{
  "format": 1,
  "type": "lycan-app",
  "id": "lycan-terminal",
  "name": "LYCAN Terminal",
  "version": "1.0.0",
  "entry": "app/app.html"
}
```

`checksums.sha256` contains one SHA-256 entry per file, using paths relative to the package root. The Store validates every listed checksum before installing. Package IDs may only contain lowercase letters, numbers, `.`, `_`, and `-`; packages are installed beneath the guest application's root.

LYCAN never executes an `.lypkg` merely because it was downloaded. Installation is an explicit Store action, and the package must pass manifest, path, and checksum validation first.
