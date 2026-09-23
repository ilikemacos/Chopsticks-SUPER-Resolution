#Requires -Version 5.1
<#
.SYNOPSIS
    Uninstalls Universal FrameFX.

.DESCRIPTION
    Removes the application files, Start Menu / Desktop shortcuts, and the
    per-user uninstall registry entry. By default it PRESERVES your profiles,
    backups and logs under %LOCALAPPDATA%\UniversalFrameFX. Pass -PurgeData to
    remove those too.

    This uninstaller never touches Windows system files or security settings.

.PARAMETER InstallDir
    Installation directory. Default: %LOCALAPPDATA%\Programs\UniversalFrameFX

.PARAMETER PurgeData
    Also delete profiles, backups and logs under %LOCALAPPDATA%\UniversalFrameFX.

.EXAMPLE
    .\uninstall.ps1

.EXAMPLE
    .\uninstall.ps1 -PurgeData
#>

[CmdletBinding()]
param(
    [string]$InstallDir = (Join-Path $env:LOCALAPPDATA "Programs\UniversalFrameFX"),
    [switch]$PurgeData
)

$ErrorActionPreference = "Stop"
Set-StrictMode -Version Latest

$AppName = "Universal FrameFX"
# The PowerShell edition stores profiles/backups/logs under %APPDATA% (Roaming).
$DataDir = Join-Path $env:APPDATA "UniversalFrameFX"

function Write-Step([string]$msg) { Write-Host "==> $msg" -ForegroundColor Cyan }
function Write-Ok([string]$msg)   { Write-Host "    $msg" -ForegroundColor Green }

Write-Step "Removing shortcuts"
$startMenu = Join-Path $env:APPDATA "Microsoft\Windows\Start Menu\Programs\$AppName.lnk"
$desktop   = Join-Path ([Environment]::GetFolderPath("Desktop")) "$AppName.lnk"
foreach ($lnk in @($startMenu, $desktop)) {
    if (Test-Path $lnk) { Remove-Item $lnk -Force; Write-Ok "Removed $lnk" }
}

Write-Step "Removing uninstall registry entry"
$uninstKey = "HKCU:\Software\Microsoft\Windows\CurrentVersion\Uninstall\UniversalFrameFX"
if (Test-Path $uninstKey) { Remove-Item $uninstKey -Recurse -Force; Write-Ok "Registry entry removed." }

Write-Step "Removing application files"
if (Test-Path $InstallDir) {
    Remove-Item -Path $InstallDir -Recurse -Force -ErrorAction SilentlyContinue
    Write-Ok "Removed $InstallDir"
} else {
    Write-Ok "No install directory at $InstallDir (already gone)."
}

if ($PurgeData) {
    Write-Step "Purging user data"
    if (Test-Path $DataDir) {
        Remove-Item -Path $DataDir -Recurse -Force -ErrorAction SilentlyContinue
        Write-Ok "Removed $DataDir"
    }
} else {
    Write-Host ""
    Write-Host "Your profiles, backups and logs were kept at:" -ForegroundColor Yellow
    Write-Host "  $DataDir"
    Write-Host "Re-run with -PurgeData to remove them as well." -ForegroundColor Yellow
}

Write-Host ""
Write-Host "$AppName has been uninstalled." -ForegroundColor Green
