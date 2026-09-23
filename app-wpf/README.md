# Universal FrameFX — desktop app (WPF)

A native Windows desktop application (.NET 8 / WPF) with a dark, modern UI. It
detects your GPU, reports what upscaling and frame-generation technologies can
honestly be used on your hardware, and manages per-game profiles and backups.

## What it does

- **Real GPU detection** via WMI (`Win32_VideoController`) for name, vendor,
  device IDs and driver, plus accurate VRAM read from the driver's registry key
  (`HardwareInformation.qwMemorySize`), which is correct for cards above 4 GB.
- **Honest capability matrix** for FSR 1/2/3, FSR 4, XeSS and frame generation —
  it states what is available and, when something is not, exactly why.
- **Render-resolution estimator** (clearly labelled a configuration estimate,
  never a measured frame rate).
- **Profiles** saved as JSON under `%APPDATA%\UniversalFrameFX\profiles`.
- **Game-folder backups** of config files before any change, with a refusal to
  touch anti-cheat-protected folders.

It never fabricates FPS and never claims support that is not real.

## Build locally

```powershell
dotnet build app-wpf/UniversalFrameFX.sln -c Release
dotnet run --project app-wpf/src/UniversalFrameFX/UniversalFrameFX.csproj
```

## Build a single-file release exe

```powershell
dotnet publish app-wpf/src/UniversalFrameFX/UniversalFrameFX.csproj `
  -c Release -r win-x64 --self-contained true `
  -p:PublishSingleFile=true -p:IncludeNativeLibrariesForSelfExtract=true `
  -o publish
```

The `WPF app` GitHub Actions workflow builds this on every push and attaches
`UniversalFrameFX.exe` to a GitHub Release when a `v*` tag is pushed.
