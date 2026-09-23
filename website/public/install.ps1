#Requires -Version 5.1
# Universal FrameFX installer (PowerShell edition). No admin, no compiler.
# Downloads the portable zip from the site, verifies SHA-256, extracts and
# installs per-user, adds a Start Menu shortcut and an uninstall entry.
#   iex (iwr -useb https://universal-framefx.vercel.app/install.ps1)
[CmdletBinding()]
param(
    [string]$BaseUrl = "https://universal-framefx.vercel.app",
    [string]$InstallDir = (Join-Path $env:LOCALAPPDATA "Programs\UniversalFrameFX"),
    [switch]$DesktopShortcut,
    [switch]$NoLaunch
)
$ErrorActionPreference = "Stop"
$AppName = "Universal FrameFX"
$ZipName = "UniversalFrameFX-portable.zip"
function Step($m){ Write-Host "==> $m" -ForegroundColor Cyan }
function Ok($m){ Write-Host "    $m" -ForegroundColor Green }
function Fail($m){ Write-Host "ERROR: $m" -ForegroundColor Red; exit 1 }
try { [Net.ServicePointManager]::SecurityProtocol = [Net.SecurityProtocolType]::Tls12 } catch {}

Step "Checking Windows 11"
$build = [int](Get-CimInstance Win32_OperatingSystem).BuildNumber
if ($build -lt 22000) { Fail "Windows 11 (build 22000+) required. Detected $build." }
Ok "Build $build."
if (-not [Environment]::Is64BitOperatingSystem) { Fail "A 64-bit OS is required." }
try { Add-Type -AssemblyName System.Windows.Forms -ErrorAction Stop } catch { Fail "Missing .NET Windows Forms." }
Ok "Requirements met."

New-Item -ItemType Directory -Path $InstallDir -Force | Out-Null
$tmp = Join-Path ([IO.Path]::GetTempPath()) ("ufx_" + [Guid]::NewGuid().ToString("N"))
New-Item -ItemType Directory -Path $tmp -Force | Out-Null
$zip = Join-Path $tmp $ZipName
try {
    Step "Downloading $ZipName"
    Invoke-WebRequest -Uri "$BaseUrl/$ZipName" -OutFile $zip -UseBasicParsing
    try {
        $sum = (Invoke-WebRequest -Uri "$BaseUrl/$ZipName.sha256" -UseBasicParsing -ErrorAction Stop).Content
        if ($sum -is [byte[]]) { $sum = [System.Text.Encoding]::UTF8.GetString($sum) }
        $expected = ((([string]$sum) -split '\s+' | Where-Object { $_ }) | Select-Object -First 1).Trim().ToLower()
        $actual = (Get-FileHash -Path $zip -Algorithm SHA256).Hash.ToLower()
        if ($expected -and $expected -ne $actual) { Fail "Checksum mismatch: expected $expected actual $actual" }
        if ($expected) { Ok "SHA-256 verified." }
    } catch { Write-Host "    No checksum published; skipping." -ForegroundColor Yellow }
    Step "Installing to $InstallDir"
    $x = Join-Path $tmp "x"
    Expand-Archive -Path $zip -DestinationPath $x -Force
    $app = Get-ChildItem -Path $x -Recurse -Filter "UniversalFrameFX.ps1" | Select-Object -First 1
    if (-not $app) { Fail "Archive missing UniversalFrameFX.ps1." }
    Copy-Item -Path (Join-Path $app.DirectoryName "*") -Destination $InstallDir -Recurse -Force
} finally {
    Remove-Item -Path $tmp -Recurse -Force -ErrorAction SilentlyContinue
}
$psApp = Join-Path $InstallDir "UniversalFrameFX.ps1"
if (-not (Test-Path $psApp)) { Fail "Install failed: UniversalFrameFX.ps1 missing." }
Ok "Installed application files."

Step "Creating Start Menu shortcut"
$wsh = New-Object -ComObject WScript.Shell
$lnk = Join-Path (Join-Path $env:APPDATA "Microsoft\Windows\Start Menu\Programs") "$AppName.lnk"
$sc = $wsh.CreateShortcut($lnk)
$sc.TargetPath = "powershell.exe"
$sc.Arguments = "-NoProfile -ExecutionPolicy Bypass -File `"$psApp`""
$sc.WorkingDirectory = $InstallDir
$sc.Save()
if ($DesktopShortcut) {
    $d = $wsh.CreateShortcut((Join-Path ([Environment]::GetFolderPath("Desktop")) "$AppName.lnk"))
    $d.TargetPath = "powershell.exe"
    $d.Arguments = "-NoProfile -ExecutionPolicy Bypass -File `"$psApp`""
    $d.WorkingDirectory = $InstallDir
    $d.Save()
}

Step "Registering uninstall entry"
$k = "HKCU:\Software\Microsoft\Windows\CurrentVersion\Uninstall\UniversalFrameFX"
New-Item -Path $k -Force | Out-Null
$uninst = "powershell -NoProfile -ExecutionPolicy Bypass -Command " +
    "`"Remove-Item -Recurse -Force '$InstallDir'; Remove-Item -Force '$lnk' -ErrorAction SilentlyContinue; " +
    "Remove-Item -Recurse -Force '$k'`""
Set-ItemProperty $k DisplayName $AppName
Set-ItemProperty $k DisplayVersion "0.1.0"
Set-ItemProperty $k Publisher "Universal FrameFX (open source)"
Set-ItemProperty $k InstallLocation $InstallDir
Set-ItemProperty $k UninstallString $uninst
Set-ItemProperty $k NoModify 1 -Type DWord
Set-ItemProperty $k NoRepair 1 -Type DWord
Ok "Registered (profiles under %APPDATA% are kept)."

if (-not $NoLaunch) {
    Step "Launching $AppName"
    Start-Process powershell.exe -ArgumentList "-NoProfile","-ExecutionPolicy","Bypass","-File",$psApp
}
Write-Host ""
Write-Host "$AppName installed successfully." -ForegroundColor Green
Write-Host "Location: $InstallDir"
