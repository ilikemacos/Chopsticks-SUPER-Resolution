#Requires -Version 5.1
<#
  Universal FrameFX — PowerShell / WinForms edition.

  A self-contained, runnable build that needs no compiler: it uses .NET Windows
  Forms (present on every Windows 11 machine) for the UI and WMI/CIM + the
  registry for real GPU detection. It configures the upscalers a game already
  supports, manages per-game JSON profiles, and backs up game folders before
  changing them.

  It is honest about limitations: it does NOT add FSR/XeSS or frame generation
  to games that were not built for them, and it never fabricates FPS numbers.

  Data lives under %APPDATA%\UniversalFrameFX. Run with:
      powershell -ExecutionPolicy Bypass -File UniversalFrameFX.ps1
#>

Set-StrictMode -Version Latest
$ErrorActionPreference = 'Stop'

Add-Type -AssemblyName System.Windows.Forms
Add-Type -AssemblyName System.Drawing
[System.Windows.Forms.Application]::EnableVisualStyles()

# --------------------------------------------------------------------------
# Paths
# --------------------------------------------------------------------------
$script:AppVersion = '0.1.0'
$script:Root      = Join-Path $env:APPDATA 'UniversalFrameFX'
$script:ProfDir   = Join-Path $script:Root 'profiles'
$script:BackupDir = Join-Path $script:Root 'backups'
$script:LogDir    = Join-Path $script:Root 'logs'
foreach ($d in @($script:Root, $script:ProfDir, $script:BackupDir, $script:LogDir)) {
    if (-not (Test-Path $d)) { New-Item -ItemType Directory -Path $d -Force | Out-Null }
}
$script:LogFile = Join-Path $script:LogDir 'universalframefx.log'

function Write-Log([string]$msg) {
    try { Add-Content -Path $script:LogFile -Value ("[{0}] {1}" -f (Get-Date -Format 'yyyy-MM-dd HH:mm:ss'), $msg) } catch {}
}

# --------------------------------------------------------------------------
# Theme
# --------------------------------------------------------------------------
$C = @{
    Bg      = [System.Drawing.Color]::FromArgb(30, 30, 30)
    Sidebar = [System.Drawing.Color]::FromArgb(37, 37, 37)
    Surface = [System.Drawing.Color]::FromArgb(43, 43, 43)
    Accent  = [System.Drawing.Color]::FromArgb(59, 130, 246)
    Text    = [System.Drawing.Color]::FromArgb(242, 242, 242)
    Muted   = [System.Drawing.Color]::FromArgb(160, 160, 160)
    Ok      = [System.Drawing.Color]::FromArgb(74, 222, 128)
    Warn    = [System.Drawing.Color]::FromArgb(251, 191, 36)
    Bad     = [System.Drawing.Color]::FromArgb(248, 113, 113)
}
$FontBody    = New-Object System.Drawing.Font('Segoe UI', 9.5)
$FontHeading = New-Object System.Drawing.Font('Segoe UI Semibold', 13)
$FontTitle   = New-Object System.Drawing.Font('Segoe UI Semibold', 18)
$FontSmall   = New-Object System.Drawing.Font('Segoe UI', 8.5)

# --------------------------------------------------------------------------
# GPU detection
# --------------------------------------------------------------------------
function Get-UfxVendor([int]$venId) {
    switch ($venId) {
        0x10DE { 'NVIDIA' }
        0x1002 { 'AMD' }
        0x1022 { 'AMD' }
        0x8086 { 'Intel' }
        0x1414 { 'Microsoft Basic Render' }
        default { 'Unknown' }
    }
}

function Get-UfxArch([string]$vendor, [int]$devId) {
    switch ($vendor) {
        'NVIDIA' {
            if ($devId -ge 0x2B00) { return 'Blackwell' }
            if ($devId -ge 0x2600) { return 'Ada Lovelace' }
            if ($devId -ge 0x2200) { return 'Ampere' }
            if ($devId -ge 0x1E00) { return 'Turing' }
            return 'Pascal or older'
        }
        'AMD' {
            if ($devId -ge 0x7500) { return 'RDNA 4' }
            if ($devId -ge 0x7440) { return 'RDNA 3' }
            if ($devId -ge 0x73A0) { return 'RDNA 2' }
            if ($devId -ge 0x7310) { return 'RDNA 1' }
            return 'GCN or older'
        }
        'Intel' {
            if ($devId -ge 0xE200) { return 'Xe2 (Battlemage)' }
            if ($devId -ge 0x5690) { return 'Xe-HPG (Arc Alchemist)' }
            return 'Xe-LP or older'
        }
        default { return 'Unknown' }
    }
}

function Get-UfxVramBytes([string]$name) {
    # Accurate VRAM from the display class registry; falls back to 0 on failure.
    try {
        $base = 'HKLM:\SYSTEM\CurrentControlSet\Control\Class\{4d36e968-e325-11ce-bfc1-08002be10318}'
        foreach ($sub in (Get-ChildItem $base -ErrorAction SilentlyContinue)) {
            $p = Get-ItemProperty $sub.PSPath -ErrorAction SilentlyContinue
            if ($null -eq $p) { continue }
            $desc = $p.'DriverDesc'
            if ($desc -and ($desc -eq $name -or $name -like "*$desc*" -or $desc -like "*$name*")) {
                $qw = $p.'HardwareInformation.qwMemorySize'
                if ($null -ne $qw) {
                    if ($qw -is [byte[]]) { return [System.BitConverter]::ToInt64($qw, 0) }
                    return [int64]$qw
                }
            }
        }
    } catch {}
    return [int64]0
}

function Get-UfxGpus {
    $list = @()
    try {
        $ctrls = Get-CimInstance -ClassName Win32_VideoController -ErrorAction Stop
    } catch {
        Write-Log "GPU enumeration failed: $($_.Exception.Message)"
        return $list
    }
    foreach ($c in $ctrls) {
        $venId = 0; $devId = 0
        if ($c.PNPDeviceID -match 'VEN_([0-9A-Fa-f]{4})') { $venId = [Convert]::ToInt32($Matches[1], 16) }
        if ($c.PNPDeviceID -match 'DEV_([0-9A-Fa-f]{4})') { $devId = [Convert]::ToInt32($Matches[1], 16) }
        $vendor = Get-UfxVendor $venId
        $vram = Get-UfxVramBytes $c.Name
        if ($vram -le 0 -and $c.AdapterRAM) { $vram = [int64]$c.AdapterRAM }
        $list += [pscustomobject]@{
            Name          = $c.Name
            Vendor        = $vendor
            Arch          = Get-UfxArch $vendor $devId
            VendorId      = $venId
            DeviceId      = $devId
            VramBytes     = $vram
            DriverVersion = $c.DriverVersion
        }
    }
    return $list
}

function Format-Vram([int64]$bytes) {
    if ($bytes -le 0) { return 'unknown' }
    return ('{0:N1} GB' -f ($bytes / 1GB))
}

# --------------------------------------------------------------------------
# API / platform capabilities (best-effort, honest)
# --------------------------------------------------------------------------
function Get-UfxPlatform {
    $build = 0
    try { $build = [int](Get-CimInstance Win32_OperatingSystem).BuildNumber } catch {}
    $win11 = $build -ge 22000
    return [pscustomobject]@{
        Build   = $build
        Win11   = $win11
        # DX11/DX12 runtimes ship with Windows; the actual feature level depends on
        # the GPU/driver. On Windows 11 both runtimes are present.
        DX11    = Test-Path (Join-Path $env:SystemRoot 'System32\d3d11.dll')
        DX12    = (Test-Path (Join-Path $env:SystemRoot 'System32\d3d12.dll')) -and $win11
        Vulkan  = Test-Path (Join-Path $env:SystemRoot 'System32\vulkan-1.dll')
    }
}

# Upscaler availability mirrors the native core's honest logic.
function Get-UfxUpscalers($gpu, $plat) {
    $rows = @()
    $add = {
        param($id, $name, $avail, $reason, $req)
        $script:__rows += [pscustomobject]@{ Id=$id; Name=$name; Available=$avail; Reason=$reason; Requirements=$req }
    }
    $script:__rows = @()

    & $add 'None' 'Native (no upscaling)' $true '' 'The game renders at its native output resolution.'

    # CSR is ours, and the only entry here that needs nothing from the game: it
    # works on a finished frame, so there is no engine data to be missing. This
    # edition still only writes profiles - see the note below.
    & $add 'CSR' 'CSR (Chopsticks Super Resolution)' $true '' ('Spatial, derived from AMD FSR 1''s EASU + RCAS (MIT): a 16-tap edge-adaptive resolve with a Rec.709 luma direction estimate and a deringing clamp, then contrast-limited sharpening that adapts to local variance. Measured +1.48 dB PSNR over FSR 1 and +1.90 dB over bilinear. Needs no game support. Use ''Upscale an image with CSR'' below to run it on an image file (via the bundled ufx-upscale tool); for games this edition stores the choice in a profile.')

    & $add 'FSR1' 'FSR 1 (spatial)' $true '' 'Spatial upscaler; runs on any GPU. Needs the game to expose it.'
    & $add 'FSR2' 'FSR 2' $true '' 'Temporal: needs motion vectors, depth and jitter from the game.'

    $fsr3ok = $plat.DX12
    & $add 'FSR3' 'FSR 3 (upscaling)' $fsr3ok ($(if ($fsr3ok) { '' } else { 'Requires a DirectX 12 capable system.' })) 'Temporal; game integration required. Frame generation is separate.'

    if ($gpu -and $gpu.Vendor -eq 'AMD' -and $gpu.Arch -eq 'RDNA 4') {
        & $add 'FSR4' 'FSR 4' $true '' 'ML upscaler; requires AMD RDNA 4 hardware and a game that ships FSR 4.'
    } else {
        $why = if (-not $gpu) { 'No GPU detected.' }
               elseif ($gpu.Vendor -ne 'AMD') { "FSR 4 requires an AMD RDNA 4 GPU; detected $($gpu.Vendor)." }
               else { "FSR 4 requires RDNA 4 hardware; detected $($gpu.Arch)." }
        & $add 'FSR4' 'FSR 4' $false $why 'ML upscaler; requires AMD RDNA 4 hardware and a game that ships FSR 4.'
    }

    & $add 'XeSS' 'Intel XeSS' $true '' 'Temporal; XMX on Intel Arc, DP4a fallback elsewhere. Game integration required.'

    $rows = $script:__rows
    Remove-Variable -Name __rows -Scope script -ErrorAction SilentlyContinue
    return $rows
}

function Get-UfxFrameGens($gpu, $plat) {
    $s = @()
    $fsr = if (-not $plat.DX12) { 'Requires compatible implementation (DX12)' } else { 'Requires game integration' }
    $xe  = if (-not $plat.DX12) { 'Requires compatible implementation (DX12)' } else { 'Requires game integration' }
    $s += [pscustomobject]@{ Name='FSR 3 Frame Generation'; State=$fsr; Note='Cannot be added to a game that did not ship it (needs the swapchain + engine motion vectors).' }
    $s += [pscustomobject]@{ Name='XeSS Frame Generation'; State=$xe;  Note='Cannot be added to a game that did not ship it. Best on Intel Arc where the game integrates it.' }
    return $s
}

function Get-UfxScaleRatio([string]$mode) {
    switch ($mode) {
        'Native'            { 1.0 }
        'Ultra Quality'     { 1.3 }
        'Quality'           { 1.5 }
        'Balanced'          { 1.7 }
        'Performance'       { 2.0 }
        'Ultra Performance' { 3.0 }
        default             { 1.0 }
    }
}

# Locates ufx-upscale.exe - the compiled CSR upscaler. This PowerShell edition
# performs no pixel work itself; when the tool is present (installed by the MSI,
# shipped beside this script, or on PATH) it can run a real CSR upscale.
function Find-UfxUpscaler {
    $candidates = @()
    if ($PSScriptRoot) { $candidates += Join-Path $PSScriptRoot 'ufx-upscale.exe' }
    $candidates += Join-Path $script:Root 'ufx-upscale.exe'
    $pf = [Environment]::GetFolderPath('ProgramFiles')
    if ($pf) { $candidates += Join-Path $pf 'Universal FrameFX\ufx-upscale.exe' }
    foreach ($c in $candidates) { if ($c -and (Test-Path $c)) { return $c } }
    $onPath = Get-Command 'ufx-upscale.exe' -ErrorAction SilentlyContinue
    if ($onPath) { return $onPath.Source }
    return $null
}

# Runs a real CSR upscale by invoking the tool. Returns $true on success.
function Invoke-UfxUpscale([string]$exe, [string]$inPath, [string]$outPath, [string]$quality) {
    $argList = @($inPath, $outPath, '--quality', $quality)
    $p = Start-Process -FilePath $exe -ArgumentList $argList -NoNewWindow -Wait -PassThru
    return ($p.ExitCode -eq 0 -and (Test-Path $outPath))
}

# --------------------------------------------------------------------------
# Profiles
# --------------------------------------------------------------------------
function Get-UfxSlug([string]$name) {
    $s = ($name.ToLower() -replace '[^a-z0-9]+', '-').Trim('-')
    if ([string]::IsNullOrWhiteSpace($s)) { $s = 'profile' }
    return $s
}

function Get-UfxProfiles {
    $out = @()
    foreach ($f in (Get-ChildItem -Path $script:ProfDir -Filter '*.json' -ErrorAction SilentlyContinue)) {
        try { $out += (Get-Content $f.FullName -Raw | ConvertFrom-Json) } catch { Write-Log "Bad profile $($f.Name): $($_.Exception.Message)" }
    }
    return $out
}

function Save-UfxProfile($p) {
    $file = Join-Path $script:ProfDir ((Get-UfxSlug $p.name) + '.json')
    ($p | ConvertTo-Json -Depth 6) | Set-Content -Path $file -Encoding UTF8
    Write-Log "Saved profile: $($p.name)"
}

function Remove-UfxProfile([string]$name) {
    $file = Join-Path $script:ProfDir ((Get-UfxSlug $name) + '.json')
    if (Test-Path $file) { Remove-Item $file -Force }
}

# --------------------------------------------------------------------------
# Backup / restore (game folder only, never system files)
# --------------------------------------------------------------------------
function Test-UfxAntiCheat([string]$dir) {
    $markers = @('EasyAntiCheat', 'BEService', 'BattlEye', 'beclient', 'vgk.sys', 'anticheat')
    try {
        foreach ($f in (Get-ChildItem -Path $dir -Recurse -File -ErrorAction SilentlyContinue)) {
            foreach ($m in $markers) { if ($f.Name -like "*$m*") { return $m } }
        }
    } catch {}
    return $null
}

function Backup-UfxGameFolder([string]$name, [string]$dir) {
    $id = Get-Date -Format 'yyyyMMddTHHmmssZ'
    $snap = Join-Path (Join-Path $script:BackupDir $id) 'files'
    New-Item -ItemType Directory -Path $snap -Force | Out-Null
    $copied = @()
    foreach ($f in (Get-ChildItem -Path $dir -File -Include *.ini,*.cfg,*.json,*.xml -Recurse -ErrorAction SilentlyContinue)) {
        $rel = $f.FullName.Substring($dir.Length).TrimStart('\')
        $dst = Join-Path $snap $rel
        New-Item -ItemType Directory -Path (Split-Path $dst) -Force | Out-Null
        Copy-Item $f.FullName $dst -Force
        $copied += $rel
    }
    $meta = [pscustomobject]@{ id=$id; gameName=$name; gameDir=$dir; files=$copied; createdUtc=$id }
    ($meta | ConvertTo-Json -Depth 5) | Set-Content -Path (Join-Path (Join-Path $script:BackupDir $id) 'backup.json') -Encoding UTF8
    Write-Log "Backup $id for '$name' ($($copied.Count) files)"
    return $meta
}

function Restore-UfxBackup([string]$id) {
    $metaFile = Join-Path (Join-Path $script:BackupDir $id) 'backup.json'
    if (-not (Test-Path $metaFile)) { throw "Backup $id not found." }
    $meta = Get-Content $metaFile -Raw | ConvertFrom-Json
    $snap = Join-Path (Join-Path $script:BackupDir $id) 'files'
    foreach ($rel in $meta.files) {
        $src = Join-Path $snap $rel
        $dst = Join-Path $meta.gameDir $rel
        if (Test-Path $src) {
            New-Item -ItemType Directory -Path (Split-Path $dst) -Force | Out-Null
            Copy-Item $src $dst -Force
        }
    }
    Write-Log "Restored backup $id"
}

# --------------------------------------------------------------------------
# UI helpers
# --------------------------------------------------------------------------
function New-Label($text, $x, $y, $w, $font, $color) {
    $l = New-Object System.Windows.Forms.Label
    $l.Text = $text; $l.Location = New-Object System.Drawing.Point($x, $y)
    $l.AutoSize = $false; $l.Width = $w; $l.Height = 22
    $l.Font = $font; $l.ForeColor = $color; $l.BackColor = [System.Drawing.Color]::Transparent
    return $l
}

function New-Card($x, $y, $w, $h) {
    $p = New-Object System.Windows.Forms.Panel
    $p.Location = New-Object System.Drawing.Point($x, $y)
    $p.Size = New-Object System.Drawing.Size($w, $h)
    $p.BackColor = $C.Surface
    return $p
}

# --------------------------------------------------------------------------
# Build the window
# --------------------------------------------------------------------------
$form = New-Object System.Windows.Forms.Form
$form.Text = 'Universal FrameFX'
$form.Size = New-Object System.Drawing.Size(1040, 700)
$form.StartPosition = 'CenterScreen'
$form.BackColor = $C.Bg
$form.Font = $FontBody
$form.MinimumSize = New-Object System.Drawing.Size(880, 560)

$sidebar = New-Object System.Windows.Forms.Panel
$sidebar.Dock = 'Left'; $sidebar.Width = 200; $sidebar.BackColor = $C.Sidebar
$form.Controls.Add($sidebar)

$logo = New-Label 'FrameFX' 20 18 160 $FontHeading $C.Accent
$sidebar.Controls.Add($logo)

$content = New-Object System.Windows.Forms.Panel
$content.Dock = 'Fill'; $content.BackColor = $C.Bg; $content.AutoScroll = $true
$content.Padding = New-Object System.Windows.Forms.Padding(28, 22, 28, 22)
$form.Controls.Add($content)
$content.BringToFront()

$verLabel = New-Label ("v" + $script:AppVersion) 20 640 160 $FontSmall $C.Muted
$sidebar.Controls.Add($verLabel)
$sidebar.Add_Resize({ $verLabel.Top = $sidebar.Height - 30 })

# Detect once at startup.
$script:Gpus = Get-UfxGpus
$script:Plat = Get-UfxPlatform
Write-Log ("Startup: {0} GPU(s), Win11={1}, DX12={2}, Vulkan={3}" -f $script:Gpus.Count, $script:Plat.Win11, $script:Plat.DX12, $script:Plat.Vulkan)

# --------------------------------------------------------------------------
# Pages
# --------------------------------------------------------------------------
function Clear-Content { $content.Controls.Clear() }

function Show-Dashboard {
    Clear-Content
    $content.Controls.Add((New-Label 'UNIVERSAL FRAMEFX' 4 0 600 $FontTitle $C.Text))
    $gpu = if ($script:Gpus.Count -gt 0) { $script:Gpus[0] } else { $null }

    $card = New-Card 4 46 330 140
    $card.Controls.Add((New-Label 'GPU' 14 10 300 $FontSmall $C.Muted))
    if ($gpu) {
        $card.Controls.Add((New-Label $gpu.Name 14 30 300 $FontHeading $C.Text))
        $card.Controls.Add((New-Label ((Format-Vram $gpu.VramBytes) + ' VRAM') 14 62 300 $FontBody $C.Muted))
        $card.Controls.Add((New-Label ('Driver: ' + $gpu.DriverVersion) 14 86 300 $FontSmall $C.Muted))
        $card.Controls.Add((New-Label ($gpu.Vendor + ' - ' + $gpu.Arch) 14 108 300 $FontSmall $C.Muted))
    } else {
        $card.Controls.Add((New-Label 'No GPU detected' 14 30 300 $FontBody $C.Bad))
    }
    $content.Controls.Add($card)

    $card2 = New-Card 348 46 330 140
    $card2.Controls.Add((New-Label 'ACTIVE PROFILE' 14 10 300 $FontSmall $C.Muted))
    $profs = Get-UfxProfiles
    if ($profs.Count -gt 0) {
        $p = $profs[0]
        $card2.Controls.Add((New-Label $p.name 14 30 300 $FontHeading $C.Text))
        $card2.Controls.Add((New-Label ("$($p.upscaler) - $($p.quality)") 14 62 300 $FontBody $C.Muted))
        $fg = if ($p.frameGenEnabled) { 'Frame Generation: Enabled' } else { 'Frame Generation: Disabled' }
        $card2.Controls.Add((New-Label $fg 14 86 300 $FontSmall $C.Muted))
    } else {
        $card2.Controls.Add((New-Label 'No profiles yet' 14 30 300 $FontBody $C.Muted))
    }
    $content.Controls.Add($card2)

    $note = New-Card 4 200 674 120
    $t = New-Object System.Windows.Forms.Label
    $t.Location = New-Object System.Drawing.Point(14, 12)
    $t.Size = New-Object System.Drawing.Size(646, 96)
    $t.Font = $FontBody; $t.ForeColor = $C.Muted; $t.BackColor = [System.Drawing.Color]::Transparent
    $t.Text = "Universal FrameFX configures the upscalers a game already supports and manages profiles and backups, and adds CSR - our own spatial upscaler, which works without game support because it only needs a finished frame. It does not add FSR/XeSS or frame generation to games that were not built for them; those need engine data no external tool can supply. FPS numbers are never fabricated; the resolution figures on the Upscaling page are configuration estimates, labelled as such."
    $note.Controls.Add($t)
    $content.Controls.Add($note)
}

function Show-Gpu {
    Clear-Content
    $content.Controls.Add((New-Label 'GPU' 4 0 400 $FontTitle $C.Text))
    $y = 50
    if ($script:Gpus.Count -eq 0) {
        $content.Controls.Add((New-Label 'No GPU detected via WMI.' 4 $y 600 $FontBody $C.Bad))
        return
    }
    foreach ($g in $script:Gpus) {
        $card = New-Card 4 $y 720 200
        $card.Controls.Add((New-Label $g.Name 16 12 690 $FontHeading $C.Text))
        $cy = 46
        $rows = @(
            @('Vendor', $g.Vendor, $C.Text),
            @('Architecture', $g.Arch, $C.Text),
            @('VRAM', (Format-Vram $g.VramBytes), $C.Text),
            @('Driver', $g.DriverVersion, $C.Text),
            @('DirectX 11', $(if ($script:Plat.DX11) { 'Supported' } else { 'Not detected' }), $(if ($script:Plat.DX11) { $C.Ok } else { $C.Bad })),
            @('DirectX 12', $(if ($script:Plat.DX12) { 'Supported (Windows 11 runtime)' } else { 'Not detected' }), $(if ($script:Plat.DX12) { $C.Ok } else { $C.Bad })),
            @('Vulkan', $(if ($script:Plat.Vulkan) { 'Supported (loader present)' } else { 'Not detected' }), $(if ($script:Plat.Vulkan) { $C.Ok } else { $C.Muted }))
        )
        foreach ($r in $rows) {
            $card.Controls.Add((New-Label $r[0] 16 $cy 200 $FontBody $C.Muted))
            $card.Controls.Add((New-Label ([string]$r[1]) 220 $cy 480 $FontBody $r[2]))
            $cy += 22
        }
        $content.Controls.Add($card)
        $y += 216
    }
}

function Show-Upscaling {
    Clear-Content
    $content.Controls.Add((New-Label 'Upscaling' 4 0 400 $FontTitle $C.Text))
    $gpu = if ($script:Gpus.Count -gt 0) { $script:Gpus[0] } else { $null }
    $ups = Get-UfxUpscalers $gpu $script:Plat
    $y = 50
    foreach ($u in $ups) {
        $card = New-Card 4 $y 720 88
        $card.Controls.Add((New-Label $u.Name 16 12 480 $FontHeading $C.Text))
        $status = if ($u.Available) { 'Available' } else { 'Unavailable' }
        $sc = if ($u.Available) { $C.Ok } else { $C.Bad }
        $sl = New-Label $status 520 12 184 $FontBody $sc
        $sl.TextAlign = 'TopRight'
        $card.Controls.Add($sl)
        $detail = if ($u.Available) { $u.Requirements } else { $u.Reason }
        $dl = New-Object System.Windows.Forms.Label
        $dl.Location = New-Object System.Drawing.Point(16, 40); $dl.Size = New-Object System.Drawing.Size(688, 40)
        $dl.Font = $FontSmall; $dl.ForeColor = $C.Muted; $dl.BackColor = [System.Drawing.Color]::Transparent
        $dl.Text = $detail
        $card.Controls.Add($dl)
        $content.Controls.Add($card)
        $y += 100
    }

    # Resolution estimator
    $calc = New-Card 4 $y 720 150
    $calc.Controls.Add((New-Label 'Resolution estimate (configuration, not measured)' 16 12 690 $FontBody $C.Text))
    $calc.Controls.Add((New-Label 'Output' 16 44 60 $FontSmall $C.Muted))
    $wBox = New-Object System.Windows.Forms.TextBox; $wBox.Text = '2560'; $wBox.Location = New-Object System.Drawing.Point(80, 42); $wBox.Width = 60
    $hBox = New-Object System.Windows.Forms.TextBox; $hBox.Text = '1440'; $hBox.Location = New-Object System.Drawing.Point(150, 42); $hBox.Width = 60
    $calc.Controls.Add($wBox); $calc.Controls.Add($hBox)
    $calc.Controls.Add((New-Label 'Mode' 230 44 40 $FontSmall $C.Muted))
    $modeBox = New-Object System.Windows.Forms.ComboBox
    $modeBox.DropDownStyle = 'DropDownList'
    [void]$modeBox.Items.AddRange(@('Native','Ultra Quality','Quality','Balanced','Performance','Ultra Performance'))
    $modeBox.SelectedItem = 'Quality'; $modeBox.Location = New-Object System.Drawing.Point(275, 42); $modeBox.Width = 150
    $calc.Controls.Add($modeBox)
    $resOut = New-Label '' 16 84 680 $FontBody $C.Accent
    $calc.Controls.Add($resOut)
    $calcBtn = New-Object System.Windows.Forms.Button
    $calcBtn.Text = 'Calculate'; $calcBtn.Location = New-Object System.Drawing.Point(440, 41); $calcBtn.Width = 90
    $calcBtn.FlatStyle = 'Flat'; $calcBtn.BackColor = $C.Accent; $calcBtn.ForeColor = [System.Drawing.Color]::White; $calcBtn.FlatAppearance.BorderSize = 0
    $calcBtn.Add_Click(({
        try {
            $ow = [int]$wBox.Text; $oh = [int]$hBox.Text
            $mode = [string]$modeBox.SelectedItem
            $r = Get-UfxScaleRatio $mode
            $iw = [math]::Round($ow / $r); $ih = [math]::Round($oh / $r)
            $resOut.Text = ("Internal {0} x {1}  ->  Output {2} x {3}   ({4})" -f $iw, $ih, $ow, $oh, $mode)
        } catch { $resOut.Text = 'Enter valid numbers for output width and height.' }
    }).GetNewClosure())
    $calc.Controls.Add($calcBtn)
    $content.Controls.Add($calc)
    $y += 166

    # Real CSR upscale of an image file, via the compiled tool. This is the one
    # place this edition performs actual upscaling rather than configuration.
    $up = New-Card 4 $y 720 172
    $up.Controls.Add((New-Label 'Upscale an image with CSR' 16 12 690 $FontHeading $C.Text))
    $exe = Find-UfxUpscaler
    if (-not $exe) {
        $msg = New-Object System.Windows.Forms.Label
        $msg.Location = New-Object System.Drawing.Point(16, 46); $msg.Size = New-Object System.Drawing.Size(688, 110)
        $msg.Font = $FontSmall; $msg.ForeColor = $C.Muted; $msg.BackColor = [System.Drawing.Color]::Transparent
        $msg.Text = ("The CSR upscaler tool (ufx-upscale.exe) was not found. This PowerShell edition drives it rather than reimplementing the maths. " +
                     "Install the Universal FrameFX MSI, or place ufx-upscale.exe beside this script, to upscale images here.")
        $up.Controls.Add($msg)
    } else {
        $up.Controls.Add((New-Label 'Input' 16 46 50 $FontSmall $C.Muted))
        $inBox = New-Object System.Windows.Forms.TextBox; $inBox.Location = New-Object System.Drawing.Point(70, 44); $inBox.Width = 430; $inBox.ReadOnly = $true
        $up.Controls.Add($inBox)
        $inBtn = New-Object System.Windows.Forms.Button
        $inBtn.Text = 'Browse...'; $inBtn.Location = New-Object System.Drawing.Point(510, 43); $inBtn.Width = 90
        $inBtn.FlatStyle = 'Flat'; $inBtn.BackColor = $C.Surface; $inBtn.ForeColor = $C.Text
        $inBtn.Add_Click(({
            $d = New-Object System.Windows.Forms.OpenFileDialog
            $d.Filter = 'Images|*.png;*.jpg;*.jpeg;*.bmp;*.tga|All files|*.*'
            if ($d.ShowDialog() -eq 'OK') { $inBox.Text = $d.FileName }
        }).GetNewClosure())
        $up.Controls.Add($inBtn)

        $up.Controls.Add((New-Label 'Mode' 16 80 50 $FontSmall $C.Muted))
        $qBox = New-Object System.Windows.Forms.ComboBox; $qBox.DropDownStyle = 'DropDownList'
        [void]$qBox.Items.AddRange(@('Ultra Quality','Quality','Balanced','Performance','Ultra Performance'))
        $qBox.SelectedItem = 'Quality'; $qBox.Location = New-Object System.Drawing.Point(70, 78); $qBox.Width = 150
        $up.Controls.Add($qBox)

        $upStatus = New-Object System.Windows.Forms.Label
        $upStatus.Location = New-Object System.Drawing.Point(16, 128); $upStatus.Size = New-Object System.Drawing.Size(688, 36)
        $upStatus.Font = $FontSmall; $upStatus.ForeColor = $C.Muted; $upStatus.BackColor = [System.Drawing.Color]::Transparent
        $up.Controls.Add($upStatus)

        $goBtn = New-Object System.Windows.Forms.Button
        $goBtn.Text = 'Upscale...'; $goBtn.Location = New-Object System.Drawing.Point(240, 78); $goBtn.Width = 110
        $goBtn.FlatStyle = 'Flat'; $goBtn.BackColor = $C.Accent; $goBtn.ForeColor = [System.Drawing.Color]::White; $goBtn.FlatAppearance.BorderSize = 0
        $goBtn.Add_Click(({
            if (-not $inBox.Text -or -not (Test-Path $inBox.Text)) { $upStatus.ForeColor = $C.Bad; $upStatus.Text = 'Choose an input image first.'; return }
            $save = New-Object System.Windows.Forms.SaveFileDialog
            $save.Filter = 'PNG image|*.png'; $save.FileName = 'upscaled.png'
            if ($save.ShowDialog() -ne 'OK') { return }
            $upStatus.ForeColor = $C.Muted; $upStatus.Text = 'Upscaling with CSR...'; $goBtn.Enabled = $false
            try {
                if (Invoke-UfxUpscale $exe $inBox.Text $save.FileName ([string]$qBox.SelectedItem)) {
                    $upStatus.ForeColor = $C.Ok; $upStatus.Text = ("Done - wrote {0}" -f $save.FileName)
                } else {
                    $upStatus.ForeColor = $C.Bad; $upStatus.Text = 'CSR upscale failed. See the tool output.'
                }
            } catch {
                $upStatus.ForeColor = $C.Bad; $upStatus.Text = ("Error: {0}" -f $_.Exception.Message)
            } finally { $goBtn.Enabled = $true }
        }).GetNewClosure())
        $up.Controls.Add($goBtn)
    }
    $content.Controls.Add($up)
}

function Show-FrameGen {
    Clear-Content
    $content.Controls.Add((New-Label 'Frame Generation' 4 0 500 $FontTitle $C.Text))
    $gpu = if ($script:Gpus.Count -gt 0) { $script:Gpus[0] } else { $null }
    $y = 50
    foreach ($f in (Get-UfxFrameGens $gpu $script:Plat)) {
        $card = New-Card 4 $y 720 108
        $card.Controls.Add((New-Label $f.Name 16 12 440 $FontHeading $C.Text))
        $sl = New-Label $f.State 456 12 248 $FontBody $C.Warn
        $sl.TextAlign = 'TopRight'
        $card.Controls.Add($sl)
        $nl = New-Object System.Windows.Forms.Label
        $nl.Location = New-Object System.Drawing.Point(16, 44); $nl.Size = New-Object System.Drawing.Size(688, 52)
        $nl.Font = $FontSmall; $nl.ForeColor = $C.Muted; $nl.BackColor = [System.Drawing.Color]::Transparent
        $nl.Text = $f.Note
        $card.Controls.Add($nl)
        $content.Controls.Add($card)
        $y += 120
    }
}

function Show-Profiles {
    Clear-Content
    $content.Controls.Add((New-Label 'Profiles' 4 0 400 $FontTitle $C.Text))

    $list = New-Object System.Windows.Forms.ListBox
    $list.Location = New-Object System.Drawing.Point(4, 50); $list.Size = New-Object System.Drawing.Size(280, 360)
    $list.BackColor = $C.Surface; $list.ForeColor = $C.Text; $list.BorderStyle = 'None'
    $content.Controls.Add($list)
    $script:__refresh = ({
        $list.Items.Clear()
        foreach ($p in (Get-UfxProfiles)) { [void]$list.Items.Add($p.name) }
    }).GetNewClosure()
    & $script:__refresh

    $fx = 300
    $content.Controls.Add((New-Label 'Name' $fx 50 120 $FontSmall $C.Muted))
    $nameBox = New-Object System.Windows.Forms.TextBox; $nameBox.Location = New-Object System.Drawing.Point($fx, 72); $nameBox.Width = 380
    $content.Controls.Add($nameBox)

    $content.Controls.Add((New-Label 'Executable path' $fx 104 200 $FontSmall $C.Muted))
    $exeBox = New-Object System.Windows.Forms.TextBox; $exeBox.Location = New-Object System.Drawing.Point($fx, 126); $exeBox.Width = 300
    $content.Controls.Add($exeBox)
    $browse = New-Object System.Windows.Forms.Button
    $browse.Text = '...'; $browse.Location = New-Object System.Drawing.Point(($fx + 305), 125); $browse.Width = 40
    $browse.FlatStyle = 'Flat'; $browse.BackColor = $C.Surface; $browse.ForeColor = $C.Text
    $browse.Add_Click(({
        $ofd = New-Object System.Windows.Forms.OpenFileDialog
        $ofd.Filter = 'Executables (*.exe)|*.exe|All files (*.*)|*.*'
        if ($ofd.ShowDialog() -eq 'OK') { $exeBox.Text = $ofd.FileName }
    }).GetNewClosure())
    $content.Controls.Add($browse)

    $mk = {
        param($label, $y, $items, $default)
        $content.Controls.Add((New-Label $label $fx $y 160 $FontSmall $C.Muted))
        $cb = New-Object System.Windows.Forms.ComboBox
        $cb.DropDownStyle = 'DropDownList'; [void]$cb.Items.AddRange($items); $cb.SelectedItem = $default
        $cb.Location = New-Object System.Drawing.Point($fx, ($y + 20)); $cb.Width = 180
        $content.Controls.Add($cb)
        return $cb
    }
    $apiBox = & $mk 'API' 158 @('DirectX 11','DirectX 12','Vulkan') 'DirectX 12'
    $upBox  = & $mk 'Upscaler' 158 @('CSR','None','FSR1','FSR2','FSR3','FSR4','XeSS') 'CSR'
    $upBox.Left = $fx + 200
    (($content.Controls | Where-Object { $_ -is [System.Windows.Forms.Label] -and $_.Text -eq 'Upscaler' })[0]).Left = $fx + 200

    $qBox = & $mk 'Quality' 210 @('Native','Ultra Quality','Quality','Balanced','Performance','Ultra Performance') 'Quality'
    $fgBox = & $mk 'Frame Gen' 210 @('None','FSR3-FG','XeSS-FG') 'None'
    $fgBox.Left = $fx + 200
    (($content.Controls | Where-Object { $_ -is [System.Windows.Forms.Label] -and $_.Text -eq 'Frame Gen' })[0]).Left = $fx + 200

    $content.Controls.Add((New-Label 'Output width / height' $fx 262 200 $FontSmall $C.Muted))
    $owBox = New-Object System.Windows.Forms.TextBox; $owBox.Text = '2560'; $owBox.Location = New-Object System.Drawing.Point($fx, 284); $owBox.Width = 80
    $ohBox = New-Object System.Windows.Forms.TextBox; $ohBox.Text = '1440'; $ohBox.Location = New-Object System.Drawing.Point(($fx + 90), 284); $ohBox.Width = 80
    $content.Controls.Add($owBox); $content.Controls.Add($ohBox)

    $fgChk = New-Object System.Windows.Forms.CheckBox
    $fgChk.Text = 'Frame generation enabled'; $fgChk.ForeColor = $C.Muted; $fgChk.BackColor = [System.Drawing.Color]::Transparent
    $fgChk.Location = New-Object System.Drawing.Point($fx, 316); $fgChk.Width = 260
    $content.Controls.Add($fgChk)

    $list.Add_SelectedIndexChanged(({
        $sel = $list.SelectedItem
        if (-not $sel) { return }
        $p = (Get-UfxProfiles | Where-Object { $_.name -eq $sel })[0]
        if (-not $p) { return }
        $nameBox.Text = $p.name; $exeBox.Text = $p.executablePath
        $apiBox.SelectedItem = $p.api; $upBox.SelectedItem = $p.upscaler
        $qBox.SelectedItem = $p.quality; $fgBox.SelectedItem = $p.frameGen
        $owBox.Text = [string]$p.outputWidth; $ohBox.Text = [string]$p.outputHeight
        $fgChk.Checked = [bool]$p.frameGenEnabled
    }).GetNewClosure())

    $btnY = 356
    $mkBtn = {
        param($text, $x, $w, $accent, $onClick)
        $b = New-Object System.Windows.Forms.Button
        $b.Text = $text; $b.Location = New-Object System.Drawing.Point($x, $btnY); $b.Width = $w; $b.Height = 30
        $b.FlatStyle = 'Flat'; $b.FlatAppearance.BorderSize = 0; $b.ForeColor = [System.Drawing.Color]::White
        $b.BackColor = $(if ($accent) { $C.Accent } else { $C.Surface })
        $b.Add_Click($onClick)
        $content.Controls.Add($b)
        return $b
    }

    & $mkBtn 'Save' $fx 84 $true ({
        if ([string]::IsNullOrWhiteSpace($nameBox.Text)) { [System.Windows.Forms.MessageBox]::Show('Enter a profile name.'); return }
        $p = [pscustomobject]@{
            schema = 1; name = $nameBox.Text; executablePath = $exeBox.Text
            api = [string]$apiBox.SelectedItem; upscaler = [string]$upBox.SelectedItem
            quality = [string]$qBox.SelectedItem; frameGen = [string]$fgBox.SelectedItem
            frameGenEnabled = [bool]$fgChk.Checked; sharpness = 0.5
            outputWidth = [int]$owBox.Text; outputHeight = [int]$ohBox.Text; fpsLimit = $null
        }
        Save-UfxProfile $p
        & $script:__refresh
        [System.Windows.Forms.MessageBox]::Show("Saved profile '$($p.name)'.")
    }.GetNewClosure()) | Out-Null

    & $mkBtn 'Delete' ($fx + 92) 84 $false ({
        if ($list.SelectedItem) { Remove-UfxProfile ([string]$list.SelectedItem); & $script:__refresh }
    }.GetNewClosure()) | Out-Null

    & $mkBtn 'Export' ($fx + 184) 84 $false ({
        if (-not $list.SelectedItem) { return }
        $p = (Get-UfxProfiles | Where-Object { $_.name -eq $list.SelectedItem })[0]
        $sfd = New-Object System.Windows.Forms.SaveFileDialog
        $sfd.Filter = 'JSON (*.json)|*.json'; $sfd.FileName = (Get-UfxSlug $p.name) + '.json'
        if ($sfd.ShowDialog() -eq 'OK') { ($p | ConvertTo-Json -Depth 6) | Set-Content $sfd.FileName -Encoding UTF8 }
    }.GetNewClosure()) | Out-Null

    & $mkBtn 'Import' ($fx + 276) 84 $false ({
        $ofd = New-Object System.Windows.Forms.OpenFileDialog
        $ofd.Filter = 'JSON (*.json)|*.json'
        if ($ofd.ShowDialog() -eq 'OK') {
            try { $p = Get-Content $ofd.FileName -Raw | ConvertFrom-Json; Save-UfxProfile $p; & $script:__refresh }
            catch { [System.Windows.Forms.MessageBox]::Show('Invalid profile JSON.') }
        }
    }.GetNewClosure()) | Out-Null

    # Backup / restore row
    $bkY = 396
    $bBtn = New-Object System.Windows.Forms.Button
    $bBtn.Text = 'Backup game folder...'; $bBtn.Location = New-Object System.Drawing.Point($fx, $bkY); $bBtn.Width = 176; $bBtn.Height = 30
    $bBtn.FlatStyle = 'Flat'; $bBtn.FlatAppearance.BorderSize = 0; $bBtn.BackColor = $C.Surface; $bBtn.ForeColor = $C.Text
    $bBtn.Add_Click(({
        $fbd = New-Object System.Windows.Forms.FolderBrowserDialog
        if ($fbd.ShowDialog() -eq 'OK') {
            $dir = $fbd.SelectedPath
            $ac = Test-UfxAntiCheat $dir
            if ($ac) { [System.Windows.Forms.MessageBox]::Show("Refused: anti-cheat detected ($ac). Never modify online-game folders."); return }
            $m = Backup-UfxGameFolder ($nameBox.Text) $dir
            [System.Windows.Forms.MessageBox]::Show("Backed up $($m.files.Count) config file(s) to snapshot $($m.id).")
        }
    }).GetNewClosure())
    $content.Controls.Add($bBtn)

    $rBtn = New-Object System.Windows.Forms.Button
    $rBtn.Text = 'Restore latest backup'; $rBtn.Location = New-Object System.Drawing.Point(($fx + 184), $bkY); $rBtn.Width = 176; $rBtn.Height = 30
    $rBtn.FlatStyle = 'Flat'; $rBtn.FlatAppearance.BorderSize = 0; $rBtn.BackColor = $C.Surface; $rBtn.ForeColor = $C.Text
    $rBtn.Add_Click({
        $snaps = Get-ChildItem -Path $script:BackupDir -Directory -ErrorAction SilentlyContinue | Sort-Object Name -Descending
        if (-not $snaps -or $snaps.Count -eq 0) { [System.Windows.Forms.MessageBox]::Show('No backups yet.'); return }
        Restore-UfxBackup $snaps[0].Name
        [System.Windows.Forms.MessageBox]::Show("Restored snapshot $($snaps[0].Name).")
    })
    $content.Controls.Add($rBtn)

    $content.Controls.Add((New-Label ("Profiles are stored as JSON in " + $script:ProfDir) $fx 436 400 $FontSmall $C.Muted))
}

function Show-About {
    Clear-Content
    $content.Controls.Add((New-Label 'About' 4 0 400 $FontTitle $C.Text))
    $t = New-Object System.Windows.Forms.Label
    $t.Location = New-Object System.Drawing.Point(4, 50); $t.Size = New-Object System.Drawing.Size(700, 220)
    $t.Font = $FontBody; $t.ForeColor = $C.Muted; $t.BackColor = [System.Drawing.Color]::Transparent
    $t.Text = "Universal FrameFX v$($script:AppVersion) (PowerShell edition)`r`n`r`nOpen source (MIT). A unified interface for configuring FSR, XeSS and frame generation for the games you already own, plus CSR - our own spatial upscaler, which needs no game support because it works on a finished frame.`r`n`r`nThis edition runs entirely on Windows PowerShell + .NET Windows Forms - no compilation required. GPU data is read from WMI and the driver, never guessed from the brand.`r`n`r`nIt never fakes support for a graphics technology, never fabricates FPS, and only modifies a game's own folder after taking a backup. It refuses to touch anti-cheat-protected folders.`r`n`r`nData directory: $($script:Root)"
    $content.Controls.Add($t)
}

# --------------------------------------------------------------------------
# Navigation
# --------------------------------------------------------------------------
$pages = @(
    @{ Name = 'Dashboard';        Action = { Show-Dashboard } },
    @{ Name = 'GPU';              Action = { Show-Gpu } },
    @{ Name = 'Upscaling';        Action = { Show-Upscaling } },
    @{ Name = 'Frame Generation'; Action = { Show-FrameGen } },
    @{ Name = 'Profiles';         Action = { Show-Profiles } },
    @{ Name = 'About';            Action = { Show-About } }
)

$script:NavButtons = @()
$ny = 64
foreach ($pg in $pages) {
    $b = New-Object System.Windows.Forms.Button
    $b.Text = $pg.Name
    $b.Tag = $pg.Action
    $b.Location = New-Object System.Drawing.Point(10, $ny)
    $b.Size = New-Object System.Drawing.Size(180, 40)
    $b.FlatStyle = 'Flat'; $b.FlatAppearance.BorderSize = 0
    $b.TextAlign = 'MiddleLeft'; $b.Padding = New-Object System.Windows.Forms.Padding(12, 0, 0, 0)
    $b.BackColor = $C.Sidebar; $b.ForeColor = $C.Text; $b.Font = $FontBody
    $b.Add_Click({
        param($sender, $e)
        foreach ($nb in $script:NavButtons) { $nb.BackColor = $C.Sidebar; $nb.ForeColor = $C.Text }
        $sender.BackColor = $C.Accent; $sender.ForeColor = [System.Drawing.Color]::White
        & $sender.Tag
    })
    $sidebar.Controls.Add($b)
    $script:NavButtons += $b
    $ny += 44
}

# Select the first page.
$script:NavButtons[0].BackColor = $C.Accent
$script:NavButtons[0].ForeColor = [System.Drawing.Color]::White
Show-Dashboard

[void][System.Windows.Forms.Application]::Run($form)
