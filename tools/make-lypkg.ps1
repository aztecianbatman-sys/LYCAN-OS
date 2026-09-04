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
Compress-Archive -Path (Join-Path $app '*') -DestinationPath $out -CompressionLevel Optimal
Write-Host "Created $out"
