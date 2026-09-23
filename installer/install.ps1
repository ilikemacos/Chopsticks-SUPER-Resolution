#Requires -Version 5.1
<#
.SYNOPSIS
    Installs Universal FrameFX from the official GitHub Releases page.

.DESCRIPTION
    This installer:
      1.  Verifies Windows 11 (build 22000+).
      2.  Verifies a 64-bit (x64) OS.
      3.  Checks required components.
      4.  Creates a per-user installation directory (no admin required).
      5.  Downloads only the release artifact from the official GitHub Releases page.
      6.  Verifies the SHA-256 checksum.
      7.  Extracts and installs the application.
      8.  Creates a Start Menu shortcut.
      9.  Optionally creates a Desktop shortcut (-DesktopShortcut).
     10.  Registers an uninstall entry (HKCU) so it appears in Apps & features.
     11.  Verifies the installation.
     12.  Optionally launches the app (-Launch).

    It does NOT: use hidden downloads, obfuscation, persistence, elevation it
    does not need, or touch Windows Defender / SmartScreen / security settings.

.PARAMETER Version
    Release tag to install (default: latest).

.PARAMETER InstallDir
    Target directory. Default: %LOCALAPPDATA%\Programs\UniversalFrameFX

.PARAMETER DesktopShortcut
    Also create a Desktop shortcut.

.PARAMETER Launch
    Launch the application after installation.

.EXAMPLE
    .\install.ps1

.EXAMPLE
    .\install.ps1 -Version v0.1.0 -DesktopShortcut -Launch
#>

[CmdletBinding()]
param(
    [string]$Version = "latest",
    [string]$InstallDir = (Join-Path $env:LOCALAPPDATA "Programs\UniversalFrameFX"),
    [switch]$DesktopShortcut,
    [switch]$Launch
)

$ErrorActionPreference = "Stop"
Set-StrictMode -Version Latest

$Owner   = "OWNER"          # <-- replace with the GitHub owner/org before publishing
$Repo    = "UniversalFrameFX"
$AppName = "Universal FrameFX"
$ExeName = "UniversalFrameFX.exe"

function Write-Step([string]$msg) { Write-Host "==> $msg" -ForegroundColor Cyan }
function Write-Ok([string]$msg)   { Write-Host "    $msg" -ForegroundColor Green }
function Write-Warn2([string]$msg){ Write-Host "    $msg" -ForegroundColor Yellow }
function Fail([string]$msg) {
    Write-Host "ERROR: $msg" -ForegroundColor Red
    Write-Host "Installation aborted. Nothing outside '$InstallDir' was changed." -ForegroundColor Red
    exit 1
}

# --- 1. Windows 11 check ------------------------------------------------------
Write-Step "Checking Windows version"
$build = [int](Get-CimInstance Win32_OperatingSystem).BuildNumber
if ($build -lt 22000) {
    Fail "Universal FrameFX requires Windows 11 (build 22000 or newer). Detected build $build."
}
Write-Ok "Windows 11 detected (build $build)."

# --- 2. Architecture check ----------------------------------------------------
Write-Step "Checking architecture"
if ([Environment]::Is64BitOperatingSystem -ne $true) {
    Fail "A 64-bit (x64) edition of Windows is required."
}
Write-Ok "64-bit OS confirmed."

# --- 3. Component checks ------------------------------------------------------
Write-Step "Checking required components"
$psVersion = $PSVersionTable.PSVersion
Write-Ok "PowerShell $psVersion."
if (-not (Get-Command Expand-Archive -ErrorAction SilentlyContinue)) {
    Fail "Expand-Archive is not available. Please use PowerShell 5.1+ or PowerShell 7."
}
# TLS 1.2 for older Windows PowerShell.
try { [Net.ServicePointManager]::SecurityProtocol = [Net.SecurityProtocolType]::Tls12 } catch {}

# --- Resolve the release ------------------------------------------------------
Write-Step "Resolving release '$Version' from github.com/$Owner/$Repo"
$apiBase = "https://api.github.com/repos/$Owner/$Repo/releases"
$headers = @{ "User-Agent" = "UniversalFrameFX-Installer"; "Accept" = "application/vnd.github+json" }
try {
    if ($Version -eq "latest") {
        $release = Invoke-RestMethod -Uri "$apiBase/latest" -Headers $headers
    } else {
        $release = Invoke-RestMethod -Uri "$apiBase/tags/$Version" -Headers $headers
    }
} catch {
    Fail "Could not query the GitHub Releases API: $($_.Exception.Message)"
}
$tag = $release.tag_name
Write-Ok "Selected release: $tag"

$zipAsset = $release.assets | Where-Object { $_.name -like "*x64*.zip" } | Select-Object -First 1
$sumAsset = $release.assets | Where-Object { $_.name -like "*.sha256" } | Select-Object -First 1
if (-not $zipAsset) { Fail "No x64 .zip artifact found on release $tag." }

$expectedSha = $null
if ($sumAsset) {
    $sumText = (Invoke-WebRequest -Uri $sumAsset.browser_download_url -Headers $headers -UseBasicParsing).Content
    # Accept either "<hash>" or "<hash>  <filename>" formats.
    $expectedSha = ($sumText -split '\s+')[0].Trim().ToLower()
} else {
    Write-Warn2 "No .sha256 checksum published with this release. The download cannot be integrity-verified."
    $answer = Read-Host "Continue without checksum verification? (y/N)"
    if ($answer -ne "y") { Fail "Aborted at user request (no checksum)." }
}

# --- 4. Prepare install / temp dirs ------------------------------------------
Write-Step "Preparing directories"
$tmp = Join-Path ([IO.Path]::GetTempPath()) ("ufx_" + [Guid]::NewGuid().ToString("N"))
New-Item -ItemType Directory -Path $tmp -Force | Out-Null
$zipPath = Join-Path $tmp $zipAsset.name
Write-Ok "Temp: $tmp"

try {
    # --- 5. Download ----------------------------------------------------------
    Write-Step "Downloading $($zipAsset.name) ($([math]::Round($zipAsset.size/1MB,1)) MB)"
    Invoke-WebRequest -Uri $zipAsset.browser_download_url -OutFile $zipPath -Headers $headers -UseBasicParsing
    Write-Ok "Downloaded."

    # --- 6. Verify checksum ---------------------------------------------------
    if ($expectedSha) {
        Write-Step "Verifying SHA-256"
        $actual = (Get-FileHash -Path $zipPath -Algorithm SHA256).Hash.ToLower()
        if ($actual -ne $expectedSha) {
            Fail "Checksum mismatch!`n  expected: $expectedSha`n  actual:   $actual"
        }
        Write-Ok "Checksum verified: $actual"
    }

    # --- 7. Install -----------------------------------------------------------
    Write-Step "Installing to $InstallDir"
    if (Test-Path $InstallDir) {
        Write-Warn2 "Existing installation found; it will be replaced (profiles under %LOCALAPPDATA%\UniversalFrameFX are untouched)."
        Get-ChildItem -Path $InstallDir -Recurse -ErrorAction SilentlyContinue | Remove-Item -Recurse -Force -ErrorAction SilentlyContinue
    }
    New-Item -ItemType Directory -Path $InstallDir -Force | Out-Null
    Expand-Archive -Path $zipPath -DestinationPath $InstallDir -Force
    Write-Ok "Extracted."

    $exePath = Join-Path $InstallDir $ExeName
    if (-not (Test-Path $exePath)) {
        # Handle a nested top-level folder in the zip.
        $found = Get-ChildItem -Path $InstallDir -Recurse -Filter $ExeName | Select-Object -First 1
        if ($found) { $exePath = $found.FullName } else { Fail "$ExeName not found after extraction." }
    }

    # --- 8. Start Menu shortcut ----------------------------------------------
    Write-Step "Creating Start Menu shortcut"
    $startMenu = Join-Path $env:APPDATA "Microsoft\Windows\Start Menu\Programs"
    $lnk = Join-Path $startMenu "$AppName.lnk"
    $wsh = New-Object -ComObject WScript.Shell
    $sc = $wsh.CreateShortcut($lnk)
    $sc.TargetPath = $exePath
    $sc.WorkingDirectory = (Split-Path $exePath)
    $sc.Description = $AppName
    $sc.Save()
    Write-Ok "Start Menu shortcut created."

    # --- 9. Desktop shortcut (optional) --------------------------------------
    if ($DesktopShortcut) {
        Write-Step "Creating Desktop shortcut"
        $desktop = [Environment]::GetFolderPath("Desktop")
        $dlnk = Join-Path $desktop "$AppName.lnk"
        $dsc = $wsh.CreateShortcut($dlnk)
        $dsc.TargetPath = $exePath
        $dsc.WorkingDirectory = (Split-Path $exePath)
        $dsc.Save()
        Write-Ok "Desktop shortcut created."
    }

    # --- 10. Uninstall entry (HKCU, no admin) --------------------------------
    Write-Step "Registering uninstall entry"
    $uninstKey = "HKCU:\Software\Microsoft\Windows\CurrentVersion\Uninstall\UniversalFrameFX"
    New-Item -Path $uninstKey -Force | Out-Null
    $uninstScript = Join-Path $InstallDir "uninstall.ps1"
    # Ship the uninstaller alongside the app if the release did not include it.
    if (-not (Test-Path $uninstScript)) {
        $thisUninstall = Join-Path $PSScriptRoot "uninstall.ps1"
        if (Test-Path $thisUninstall) { Copy-Item $thisUninstall $uninstScript -Force }
    }
    Set-ItemProperty -Path $uninstKey -Name "DisplayName" -Value $AppName
    Set-ItemProperty -Path $uninstKey -Name "DisplayVersion" -Value ($tag.TrimStart("v"))
    Set-ItemProperty -Path $uninstKey -Name "Publisher" -Value "Universal FrameFX (open source)"
    Set-ItemProperty -Path $uninstKey -Name "InstallLocation" -Value $InstallDir
    Set-ItemProperty -Path $uninstKey -Name "DisplayIcon" -Value $exePath
    Set-ItemProperty -Path $uninstKey -Name "NoModify" -Value 1 -Type DWord
    Set-ItemProperty -Path $uninstKey -Name "NoRepair" -Value 1 -Type DWord
    if (Test-Path $uninstScript) {
        Set-ItemProperty -Path $uninstKey -Name "UninstallString" `
            -Value "powershell -ExecutionPolicy Bypass -File `"$uninstScript`""
    }
    Write-Ok "Uninstall entry registered (per-user)."

    # --- 11. Verify -----------------------------------------------------------
    Write-Step "Verifying installation"
    if (-not (Test-Path $exePath)) { Fail "Verification failed: $ExeName missing." }
    Write-Ok "Verified: $exePath"

    # --- 12. Launch (optional) -----------------------------------------------
    if ($Launch) {
        Write-Step "Launching $AppName"
        Start-Process -FilePath $exePath
    }

    Write-Host ""
    Write-Host "$AppName $tag installed successfully." -ForegroundColor Green
    Write-Host "Location: $InstallDir"
}
finally {
    Remove-Item -Path $tmp -Recurse -Force -ErrorAction SilentlyContinue
}
