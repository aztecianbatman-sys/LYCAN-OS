# LYPKG package format

`*.lypkg` is a ZIP-compatible archive using **stored (uncompressed) entries**. This keeps the runtime dependency-free while allowing normal archive tooling to inspect packages.

```text
terminal-1.0.0.lypkg
├── manifest.json
├── app/
│   └── terminal
└── checksums.sha256
```

## manifest.json

```json
{
  "id": "lycan-terminal",
  "name": "Terminal",
  "version": "1.0.0",
  "publisher": "LYCAN",
  "entry": "/apps/lycan-terminal/terminal",
  "permissions": [
    "lyfs.read",
    "lyfs.write"
  ]
}
```

The package manager validates the package ID, publisher trust, entry-point containment, archive paths, ZIP CRC values, and the manifest before installation. Package payloads are extracted only below `/apps/<id>/`.

`checksums.sha256` contains one SHA-256 digest per application payload:

```text
<sha256>  app/terminal
```

Third-party publishers can use the same format. Publisher trust remains a separate LYCAN security-policy decision; merely possessing a `.lypkg` file does not grant installation or guest capabilities.
