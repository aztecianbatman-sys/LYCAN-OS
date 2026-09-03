param([string]$SourceRoot = (Split-Path -Parent $PSScriptRoot), [string]$InstallRoot = "$env:LOCALAPPDATA\LYCAN-OS")
$ErrorActionPreference='Stop'
New-Item -ItemType Directory -Force -Path $InstallRoot | Out-Null
Get-ChildItem -LiteralPath $SourceRoot -Force | Where-Object { $_.Name -ne 'install.ps1' } | ForEach-Object { Copy-Item $_.FullName -Destination (Join-Path $InstallRoot $_.Name) -Recurse -Force }
$marker=Join-Path $InstallRoot 'LYCAN-WINDOWS-HOSTED.marker'
Set-Content -Path $marker -Value "LYCAN $([DateTime]::UtcNow.ToString('o'))`nHosted by Windows; no boot-sector or partition changes are performed.`n"
Write-Host "LYCAN installed to $InstallRoot"
Write-Host "This installer copies the VM/runtime only; it does not replace Windows."
