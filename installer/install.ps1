#Requires -Version 5.1
<#
.SYNOPSIS
    Install the Universal FrameFX PowerShell edition (no admin, no compiler).

.DESCRIPTION
    Downloads the portable zip from the official Universal FrameFX website,
    verifies its SHA-256, extracts the runnable PowerShell/WinForms app, and
    installs it per-user. It:

      1.  Verifies Windows 11.
      2.  Verifies a 64-bit OS.
      3.  Checks PowerShell + .NET Windows Forms are available.
      4.  Creates a per-user install directory (no elevation required).
      5.  Downloads UniversalFrameFX-portable.zip over HTTPS.
      6.  Verifies its SHA-256 when a checksum is published.
      7.  Extracts and installs the app.
      8.  Creates a Start Menu shortcut.
      9.  Optionally creates a Desktop shortcut (-DesktopShortcut).
     10.  Registers an uninstall entry (per-user; appears in Apps & features).
     11.  Verifies the installation.
     12.  Optionally launches the app (default on).

    It does NOT: use hidden downloads, obfuscation, persistence, elevation it
    does not need, or change Windows Defender / SmartScreen / security settings.

    Run it straight from the website:
        iex (iwr -useb https://universal-framefx.vercel.app/install.ps1)

.PARAMETER BaseUrl
    Where to fetch the app from (default: the official site).

.PARAMETER InstallDir
    Target directory (default: %LOCALAPPDATA%\Programs\UniversalFrameFX).

.PARAMETER DesktopShortcut
    Also create a Desktop shortcut.

.PARAMETER NoLaunch
    Do not launch the app after installing.
#>

[CmdletBinding()]
param(
    [string]$BaseUrl = "https://universal-framefx.vercel.app",
    [string]$InstallDir = (Join-Path $env:LOCALAPPDATA "Programs\UniversalFrameFX"),
    [switch]$DesktopShortcut,
    [switch]$NoLaunch
)

$ErrorActionPreference = "Stop"
Set-StrictMode -Version Latest

$AppName = "Universal FrameFX"
$ZipName = "UniversalFrameFX-portable.zip"

function Write-Step([string]$m) { Write-Host "==> $m" -ForegroundColor Cyan }
function Write-Ok([string]$m)   { Write-Host "    $m" -ForegroundColor Green }
function Write-Warn2([string]$m){ Write-Host "    $m" -ForegroundColor Yellow }
function Fail([string]$m) {
    Write-Host "ERROR: $m" -ForegroundColor Red
    Write-Host "Installation aborted. Nothing outside '$InstallDir' was changed." -ForegroundColor Red
    exit 1
}

try { [Net.ServicePointManager]::SecurityProtocol = [Net.SecurityProtocolType]::Tls12 } catch {}

# --- 1. Windows 11 -----------------------------------------------------------
Write-Step "Checking Windows version"
$build = [int](Get-CimInstance Win32_OperatingSystem).BuildNumber
if ($build -lt 22000) { Fail "Universal FrameFX requires Windows 11 (build 22000+). Detected build $build." }
Write-Ok "Windows 11 detected (build $build)."

# --- 2. Architecture ---------------------------------------------------------
Write-Step "Checking architecture"
if (-not [Environment]::Is64BitOperatingSystem) { Fail "A 64-bit (x64) edition of Windows is required." }
Write-Ok "64-bit OS confirmed."

# --- 3. Components -----------------------------------------------------------
Write-Step "Checking required components"
Write-Ok "PowerShell $($PSVersionTable.PSVersion)."
try { Add-Type -AssemblyName System.Windows.Forms -ErrorAction Stop; Write-Ok ".NET Windows Forms available." }
catch { Fail "This system is missing .NET Windows Forms, which the app requires." }

# --- 4. Directory ------------------------------------------------------------
Write-Step "Preparing install directory"
New-Item -ItemType Directory -Path $InstallDir -Force | Out-Null
Write-Ok $InstallDir

# --- 5. Download the portable zip -------------------------------------------
$tmp = Join-Path ([IO.Path]::GetTempPath()) ("ufx_" + [Guid]::NewGuid().ToString("N"))
New-Item -ItemType Directory -Path $tmp -Force | Out-Null
$zipPath = Join-Path $tmp $ZipName
try {
    Write-Step "Downloading $ZipName"
    Invoke-WebRequest -Uri "$BaseUrl/$ZipName" -OutFile $zipPath -UseBasicParsing
    Write-Ok "Downloaded."

    # --- 6. Verify checksum --------------------------------------------------
    try {
        $sumText = (Invoke-WebRequest -Uri "$BaseUrl/$ZipName.sha256" -UseBasicParsing -ErrorAction Stop).Content
        if ($sumText -is [byte[]]) { $sumText = [System.Text.Encoding]::UTF8.GetString($sumText) }
        $expected = ((([string]$sumText) -split '\s+' | Where-Object { $_ }) | Select-Object -First 1).Trim().ToLower()
        $actual = (Get-FileHash -Path $zipPath -Algorithm SHA256).Hash.ToLower()
        if ($expected -and $expected -ne $actual) {
            Fail "Checksum mismatch:`n  expected $expected`n  actual   $actual"
        }
        if ($expected) { Write-Ok "SHA-256 verified: $actual" }
    } catch {
        Write-Warn2 "No published checksum; skipping integrity check."
    }

    # --- 7. Extract & install ------------------------------------------------
    Write-Step "Installing to $InstallDir"
    $extract = Join-Path $tmp "x"
    Expand-Archive -Path $zipPath -DestinationPath $extract -Force
    # The zip contains a top-level UniversalFrameFX folder.
    $srcApp = Get-ChildItem -Path $extract -Recurse -Filter "UniversalFrameFX.ps1" | Select-Object -First 1
    if (-not $srcApp) { Fail "Archive did not contain UniversalFrameFX.ps1." }
    Copy-Item -Path (Join-Path $srcApp.DirectoryName "*") -Destination $InstallDir -Recurse -Force
    $psApp = Join-Path $InstallDir "UniversalFrameFX.ps1"
    if (-not (Test-Path $psApp)) { Fail "Installation failed: UniversalFrameFX.ps1 missing." }
    Write-Ok "Installed application files."
}
finally {
    Remove-Item -Path $tmp -Recurse -Force -ErrorAction SilentlyContinue
}

# --- 8. Start Menu shortcut --------------------------------------------------
Write-Step "Creating Start Menu shortcut"
$wsh = New-Object -ComObject WScript.Shell
$startMenu = Join-Path $env:APPDATA "Microsoft\Windows\Start Menu\Programs"
$lnk = Join-Path $startMenu "$AppName.lnk"
$sc = $wsh.CreateShortcut($lnk)
$sc.TargetPath = "powershell.exe"
$sc.Arguments = "-NoProfile -ExecutionPolicy Bypass -File `"$psApp`""
$sc.WorkingDirectory = $InstallDir
$sc.Description = $AppName
$sc.Save()
Write-Ok "Start Menu shortcut created."

# --- 9. Desktop shortcut -----------------------------------------------------
if ($DesktopShortcut) {
    Write-Step "Creating Desktop shortcut"
    $dlnk = Join-Path ([Environment]::GetFolderPath("Desktop")) "$AppName.lnk"
    $dsc = $wsh.CreateShortcut($dlnk)
    $dsc.TargetPath = "powershell.exe"
    $dsc.Arguments = "-NoProfile -ExecutionPolicy Bypass -File `"$psApp`""
    $dsc.WorkingDirectory = $InstallDir
    $dsc.Save()
    Write-Ok "Desktop shortcut created."
}

# --- 10. Uninstall entry (per-user) -----------------------------------------
Write-Step "Registering uninstall entry"
$uninstKey = "HKCU:\Software\Microsoft\Windows\CurrentVersion\Uninstall\UniversalFrameFX"
New-Item -Path $uninstKey -Force | Out-Null
$uninstallCmd = "powershell -NoProfile -ExecutionPolicy Bypass -Command " +
    "`"Remove-Item -Recurse -Force '$InstallDir'; " +
    "Remove-Item -Force '$lnk' -ErrorAction SilentlyContinue; " +
    "Remove-Item -Recurse -Force 'HKCU:\Software\Microsoft\Windows\CurrentVersion\Uninstall\UniversalFrameFX'`""
Set-ItemProperty -Path $uninstKey -Name "DisplayName" -Value $AppName
Set-ItemProperty -Path $uninstKey -Name "DisplayVersion" -Value "0.1.0"
Set-ItemProperty -Path $uninstKey -Name "Publisher" -Value "Universal FrameFX (open source)"
Set-ItemProperty -Path $uninstKey -Name "InstallLocation" -Value $InstallDir
Set-ItemProperty -Path $uninstKey -Name "UninstallString" -Value $uninstallCmd
Set-ItemProperty -Path $uninstKey -Name "NoModify" -Value 1 -Type DWord
Set-ItemProperty -Path $uninstKey -Name "NoRepair" -Value 1 -Type DWord
Write-Ok "Uninstall entry registered (per-user; profiles under %APPDATA% are kept)."

# --- 11. Verify --------------------------------------------------------------
Write-Step "Verifying installation"
if (-not (Test-Path $psApp)) { Fail "Verification failed: UniversalFrameFX.ps1 missing." }
Write-Ok "Verified: $psApp"

# --- 12. Launch --------------------------------------------------------------
if (-not $NoLaunch) {
    Write-Step "Launching $AppName"
    Start-Process -FilePath "powershell.exe" -ArgumentList "-NoProfile","-ExecutionPolicy","Bypass","-File",$psApp
}

Write-Host ""
Write-Host "$AppName installed successfully." -ForegroundColor Green
Write-Host "Location: $InstallDir"
Write-Host "Start Menu: $AppName"
