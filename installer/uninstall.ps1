param([string]$InstallRoot = "$env:LOCALAPPDATA\LYCAN-OS")
$ErrorActionPreference='Stop'
if(Test-Path -LiteralPath $InstallRoot){Remove-Item -LiteralPath $InstallRoot -Recurse -Force}
Write-Host "LYCAN removed from $InstallRoot. Windows itself is unchanged."
