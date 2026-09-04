param(
  [Parameter(Mandatory=$true)][string]$AppDirectory,
  [Parameter(Mandatory=$true)][string]$Output
)

$ErrorActionPreference = 'Stop'
$app = (Resolve-Path $AppDirectory).Path
$manifestPath = Join-Path $app 'manifest.json'
if (!(Test-Path $manifestPath)) { throw 'manifest.json is required' }
if (!(Test-Path (Join-Path $app 'app'))) { throw 'app/ directory is required' }

$manifest = Get-Content $manifestPath -Raw | ConvertFrom-Json
if ($manifest.type -ne 'lycan-app') { throw 'manifest.type must be lycan-app' }
if ($manifest.format -ne 1) { throw 'manifest.format must be 1' }
if ($manifest.id -notmatch '^[a-z0-9][a-z0-9._-]{1,63}$') { throw 'manifest.id is invalid' }
if ($manifest.version -notmatch '^([0-9]+)\.([0-9]+)\.([0-9]+)([-+][0-9A-Za-z.-]+)?$') { throw 'manifest.version must be semver-like' }
if (!$manifest.name) { throw 'manifest.name is required' }
if ($manifest.storageQuotaMB) {
  $quota = [int]$manifest.storageQuotaMB
  if ($quota -lt 1 -or $quota -gt 1024) { throw 'manifest.storageQuotaMB must be 1-1024' }
}
if ($manifest.entry) {
  $entry = ([string]$manifest.entry).Replace('\','/')
  if ($entry.StartsWith('/') -or $entry.Contains('../') -or $entry -match '(^|/)\.\.(\/|$)' -or $entry -notlike 'app/*') { throw 'manifest.entry must stay inside app/' }
  if (!(Test-Path (Join-Path $app ($entry -replace '/','\')))) { throw 'manifest.entry does not exist' }
}
if ($manifest.icon) {
  $icon = ([string]$manifest.icon).Replace('\','/')
  if ($icon.StartsWith('/') -or $icon.Contains('../') -or $icon -match '(^|/)\.\.(\/|$)' -or $icon -notlike 'app/*') { throw 'manifest.icon must stay inside app/' }
  if (!(Test-Path (Join-Path $app ($icon -replace '/','\')))) { throw 'manifest.icon does not exist' }
}
if ($manifest.permissions) {
  $allowed = @('storage','network','external','notifications','clipboard-read','clipboard-write')
  foreach ($permission in $manifest.permissions) { if ($allowed -notcontains [string]$permission) { throw "Unsupported permission: $permission" } }
}

$checksumPath = Join-Path $app 'checksums.sha256'
$lines = @()
Get-ChildItem -LiteralPath $app -File -Recurse | Where-Object { $_.Name -ne 'checksums.sha256' } | ForEach-Object {
  $relative = $_.FullName.Substring($app.Length + 1).Replace('\','/')
  $hash = (Get-FileHash -LiteralPath $_.FullName -Algorithm SHA256).Hash.ToLowerInvariant()
  $lines += "$hash  $relative"
}
Set-Content -LiteralPath $checksumPath -Value ($lines -join "`n") -Encoding UTF8

$out = [IO.Path]::GetFullPath($Output)
if ([IO.Path]::GetExtension($out) -ne '.lypkg') { $out += '.lypkg' }
if (Test-Path $out) { Remove-Item -LiteralPath $out -Force }
$tempZip = "$out.zip"
if (Test-Path $tempZip) { Remove-Item -LiteralPath $tempZip -Force }
try {
  Compress-Archive -Path (Join-Path $app '*') -DestinationPath $tempZip -CompressionLevel Optimal
  Move-Item -LiteralPath $tempZip -Destination $out -Force
} finally {
  if (Test-Path $tempZip) { Remove-Item -LiteralPath $tempZip -Force }
}
Write-Host "Created $out"