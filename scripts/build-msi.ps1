#Requires -Version 5.1
<#
.SYNOPSIS
    Build the Universal FrameFX MSI with the WiX Toolset (v5).

.DESCRIPTION
    Stages the built application plus its payload (README, LICENSE, examples,
    capabilities.json, uninstall.ps1) into a temporary folder, then compiles
    installer/msi/Package.wxs into a per-machine x64 MSI.

    Requires:
      - The app already built (build/app/Release/UniversalFrameFX.exe), or pass
        -ExePath to point at it.
      - The WiX .NET tool. If missing, install with:
            dotnet tool install --global wix
        This script installs the WiX.UI extension automatically.

.PARAMETER Version
    Product version written into the MSI (default 0.1.0).

.PARAMETER ExePath
    Path to UniversalFrameFX.exe (default build/app/Release/UniversalFrameFX.exe).

.PARAMETER OutFile
    Output MSI path (default UniversalFrameFX-x64.msi in the repo root).

.EXAMPLE
    .\scripts\build-msi.ps1 -Version 0.1.0
#>
[CmdletBinding()]
param(
    [string]$Version = "0.1.0",
    [string]$ExePath,
    [string]$OutFile
)

$ErrorActionPreference = "Stop"
$root = Split-Path -Parent $PSScriptRoot

if (-not $ExePath)  { $ExePath  = Join-Path $root "build\app\Release\UniversalFrameFX.exe" }
if (-not $OutFile)  { $OutFile  = Join-Path $root "UniversalFrameFX-x64.msi" }

function Write-Step([string]$m) { Write-Host "==> $m" -ForegroundColor Cyan }

if (-not (Test-Path $ExePath)) {
    throw "Application executable not found at '$ExePath'. Build the app first (cmake --build build --config Release) or pass -ExePath."
}

# --- Ensure WiX is available -------------------------------------------------
Write-Step "Checking for the WiX Toolset"
if (-not (Get-Command wix -ErrorAction SilentlyContinue)) {
    Write-Host "    'wix' not found; installing the WiX .NET global tool..." -ForegroundColor Yellow
    dotnet tool install --global wix | Out-Host
    $env:PATH = "$env:PATH;$env:USERPROFILE\.dotnet\tools"
}
wix extension add -g WixToolset.UI.wixext | Out-Host

# --- Stage the payload -------------------------------------------------------
Write-Step "Staging payload"
$stage = Join-Path ([IO.Path]::GetTempPath()) ("ufx_msi_" + [Guid]::NewGuid().ToString("N"))
New-Item -ItemType Directory -Path $stage -Force | Out-Null
try {
    Copy-Item $ExePath (Join-Path $stage "UniversalFrameFX.exe") -Force
    # The CSR command-line tools live beside the app exe in the build output.
    # ufx-upscale.exe is what the app (and the PowerShell edition) shells out to
    # in order to actually upscale a file; ufx-live.exe is the CSR real-time
    # loop. Ship both, or an MSI install cannot run CSR at all.
    $exeDir = Split-Path -Parent $ExePath
    foreach ($tool in @("ufx-upscale.exe", "ufx-live.exe")) {
        $tp = Join-Path $exeDir $tool
        if (Test-Path $tp) { Copy-Item $tp $stage -Force }
        else { Write-Host "    warning: $tool not found beside the app exe; MSI will omit it." -ForegroundColor Yellow }
    }
    foreach ($f in @("README.md", "LICENSE")) {
        $p = Join-Path $root $f
        if (Test-Path $p) { Copy-Item $p $stage -Force }
    }
    $caps = Join-Path $root "app\upscaling\capabilities.json"
    if (Test-Path $caps) { Copy-Item $caps $stage -Force }
    $uninstall = Join-Path $root "installer\uninstall.ps1"
    if (Test-Path $uninstall) { Copy-Item $uninstall $stage -Force }
    $examples = Join-Path $root "examples"
    if (Test-Path $examples) { Copy-Item $examples (Join-Path $stage "examples") -Recurse -Force }

    # --- Build the MSI -------------------------------------------------------
    Write-Step "Building MSI (v$Version, x64)"
    $wxs = Join-Path $root "installer\msi\Package.wxs"
    wix build $wxs `
        -ext WixToolset.UI.wixext `
        -arch x64 `
        -d "StageDir=$stage" `
        -d "ProductVersion=$Version" `
        -o $OutFile
    if ($LASTEXITCODE -ne 0) { throw "wix build failed with exit code $LASTEXITCODE." }

    Write-Step "Done"
    $hash = (Get-FileHash -Algorithm SHA256 $OutFile).Hash.ToLower()
    Write-Host "MSI:     $OutFile" -ForegroundColor Green
    Write-Host "SHA-256: $hash" -ForegroundColor Green
}
finally {
    Remove-Item -Path $stage -Recurse -Force -ErrorAction SilentlyContinue
}
