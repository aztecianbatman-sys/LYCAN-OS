param(
  [Parameter(Mandatory=$true)][string]$Id,
  [Parameter(Mandatory=$true)][string]$Name,
  [string]$Version = '1.0.0',
  [int]$StorageQuotaMB = 16,
  [string[]]$Permissions = @('storage'),
  [string]$Output = '.'
)

$ErrorActionPreference = 'Stop'
if ($Id -notmatch '^[a-z0-9][a-z0-9._-]{1,63}$') { throw 'Id must match ^[a-z0-9][a-z0-9._-]{1,63}$' }
if ($Version -notmatch '^([0-9]+)\.([0-9]+)\.([0-9]+)([-+][0-9A-Za-z.-]+)?$') { throw 'Version must be semver-like, e.g. 1.0.0' }
if ($StorageQuotaMB -lt 1 -or $StorageQuotaMB -gt 1024) { throw 'StorageQuotaMB must be between 1 and 1024' }
$allowed = @('storage','network','external','notifications','clipboard-read','clipboard-write')
foreach($permission in $Permissions) { if($allowed -notcontains $permission) { throw "Unsupported permission: $permission" } }

$root = Join-Path (Resolve-Path $Output).Path $Id
if (Test-Path $root) { throw "Directory already exists: $root" }
New-Item -ItemType Directory -Path (Join-Path $root 'app') -Force | Out-Null
New-Item -ItemType Directory -Path (Join-Path $root 'app/assets') -Force | Out-Null

$permissionJson = (($Permissions | ForEach-Object { '    "' + $_ + '"' }) -join ",`n")
@"
{
  "format": 1,
  "type": "lycan-app",
  "id": "$Id",
  "name": "$Name",
  "version": "$Version",
  "description": "A LYCAN guest application.",
  "entry": "app/index.html",
  "icon": "app/assets/icon.svg",
  "permissions": [
$permissionJson
  ],
  "storageQuotaMB": $StorageQuotaMB
}
"@ | Set-Content -LiteralPath (Join-Path $root 'manifest.json') -Encoding UTF8

@'
<!doctype html>
<html lang="en">
<head>
<meta charset="utf-8">
<meta name="viewport" content="width=device-width,initial-scale=1">
<title>LYCAN APP</title>
<style>
html,body{height:100%;margin:0;background:#070a0f;color:#eef6ff;font-family:Segoe UI,Arial,sans-serif}
main{height:100%;display:grid;place-items:center;text-align:center}
.panel{padding:36px;border:1px solid #27384b;background:#0b1119;box-shadow:0 0 60px rgba(21,151,255,.12)}
small{letter-spacing:.18em;color:#7fa0bd}
</style>
</head>
<body><main><section class="panel"><small>LYCAN PACKAGE</small><h1>APP READY</h1><p>Your guest application is running inside LYCAN.</p></section></main></body>
</html>
'@ | Set-Content -LiteralPath (Join-Path $root 'app/index.html') -Encoding UTF8

@'
<svg xmlns="http://www.w3.org/2000/svg" viewBox="0 0 128 128">
<rect width="128" height="128" rx="24" fill="#080c12"/>
<path d="M22 88 16 34l24 18 24-28 24 28 24-18-6 54-42 24z" fill="#edf6ff"/>
<path d="m39 70 25-18 25 18-25 18z" fill="#080c12"/>
<path d="m44 59 20-9 20 9-20 7z" fill="#1597ff"/>
</svg>
'@ | Set-Content -LiteralPath (Join-Path $root 'app/assets/icon.svg') -Encoding UTF8

@"
# $Name

LYCAN guest application generated from the LYPKG starter.

## Build

```powershell
..\..\tools\make-lypkg.ps1 -AppDirectory . -Output ..\$Id-$Version.lypkg
```

The generated manifest uses a $StorageQuotaMB MB guest storage quota and permissions: $($Permissions -join ', ').

Edit `manifest.json` and the contents of `app/`, then rebuild the package.
"@ | Set-Content -LiteralPath (Join-Path $root 'README.md') -Encoding UTF8

Write-Host "Created LYPKG project: $root"
Write-Host "Storage quota: $StorageQuotaMB MB"
Write-Host "Permissions: $($Permissions -join ', ')"
Write-Host "Run tools/make-lypkg.ps1 to package it."
