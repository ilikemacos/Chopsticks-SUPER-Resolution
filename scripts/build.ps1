#Requires -Version 5.1
<#
.SYNOPSIS
    Configure, build and test Universal FrameFX from source on Windows.
.PARAMETER Config
    Build configuration: Debug or Release (default Release).
.PARAMETER NoTests
    Skip running ctest after the build.
.EXAMPLE
    .\scripts\build.ps1 -Config Release
#>
[CmdletBinding()]
param(
    [ValidateSet("Debug", "Release")]
    [string]$Config = "Release",
    [switch]$NoTests
)

$ErrorActionPreference = "Stop"
$root = Split-Path -Parent $PSScriptRoot

Write-Host "==> Configuring (x64)" -ForegroundColor Cyan
cmake -S $root -B "$root/build" -A x64

Write-Host "==> Building ($Config)" -ForegroundColor Cyan
cmake --build "$root/build" --config $Config --parallel

if (-not $NoTests) {
    Write-Host "==> Testing" -ForegroundColor Cyan
    ctest --test-dir "$root/build" -C $Config --output-on-failure
}

Write-Host "Done." -ForegroundColor Green
