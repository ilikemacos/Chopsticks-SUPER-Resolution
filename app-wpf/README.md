# Universal FrameFX — desktop app (WPF)

A native Windows desktop application (.NET 8 / WPF) with a dark, modern UI. It
detects your GPU, reports what upscaling and frame-generation technologies can
honestly be used on your hardware, and manages per-game profiles and backups.

## What it does

- **Real GPU detection** via WMI (`Win32_VideoController`) for name, vendor,
  device IDs and driver, plus accurate VRAM read from the driver's registry key
  (`HardwareInformation.qwMemorySize`), which is correct for cards above 4 GB.
- **CSR preview** on the Upscaling page: runs the real `Csr.Core` upscaler on an
  image you pick and shows it beside a pinned bilinear baseline, with the measured
  CPU time. Both filters are verified against the Python reference in `csr/ref`
  (`csr/tests/Csr.Core.Tests`), so the panes are the actual algorithms, not a
  mock-up, and the "bilinear" pane really is bilinear.
- **Honest capability matrix** for FSR 1/2/3, FSR 4, XeSS and frame generation —
  it states what is available and, when something is not, exactly why.
- **Render-resolution estimator** (clearly labelled a configuration estimate,
  never a measured frame rate).
- **Measured Direct3D feature levels** from a real `D3D11CreateDevice` /
  `D3D12CreateDevice` probe rather than looking for DLLs on disk, and an explicit
  *undetermined* state wherever the GPU architecture cannot be confirmed — an
  unconfirmed architecture is never reported as "unsupported".
- **Verified auto-update**: the downloaded exe's SHA-256 must match the `.sha256`
  published beside it or it is deleted and never executed.
- **Profiles** saved as JSON under `%APPDATA%\UniversalFrameFX\profiles`.
- **Game-folder backups** of config files before any change, with a refusal to
  touch anti-cheat-protected folders.

It never fabricates FPS and never claims support that is not real.

## Build locally

```powershell
dotnet build app-wpf/UniversalFrameFX.sln -c Release
dotnet test  app-wpf/tests/UniversalFrameFX.Tests/UniversalFrameFX.Tests.csproj -c Release
dotnet test  csr/Csr.sln -c Release
dotnet run --project app-wpf/src/UniversalFrameFX/UniversalFrameFX.csproj
```

`Csr.Core` is part of `UniversalFrameFX.sln` deliberately: as an out-of-solution
project reference it was built in `Debug` even during a `-c Release` solution
build, so the release binary shipped an unoptimised copy of the upscaler core.

## Build a single-file release exe

```powershell
dotnet publish app-wpf/src/UniversalFrameFX/UniversalFrameFX.csproj `
  -c Release -r win-x64 --self-contained true `
  -p:PublishSingleFile=true -p:IncludeNativeLibrariesForSelfExtract=true `
  -o publish
```

The `WPF app` GitHub Actions workflow builds this on every push and attaches
`UniversalFrameFX.exe` to a GitHub Release when a `v*` tag is pushed.
