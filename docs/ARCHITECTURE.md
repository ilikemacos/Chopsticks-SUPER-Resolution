# Universal FrameFX — Architecture & Implementation Plan

## What this project is (and is not)

Universal FrameFX is an **open-source Windows 11 desktop application** that:

1. Detects installed GPUs and their real capabilities.
2. Provides a unified UI for configuring upscaling (FSR, FSR 3, FSR 4 where legal, XeSS) and frame generation.
3. Manages per-game profiles as JSON.
4. Where a game already ships with an upscaler, edits its config files (with backups) to expose modes the game's own launcher does not.
5. Where a public, redistributable DLL replacement path exists (e.g. FSR3-to-DLSS or DLSS-to-FSR3 wrappers) it can drop those DLLs **into a specific game's folder** with a full backup and one-click restore.

It is **not**:

- A DLL injector that "adds FSR/XeSS to any game." That does not exist as a general capability; each upscaler needs motion vectors, depth buffers, and jitter offsets that only the game engine can provide.
- A way to bypass anti-cheat.
- A modifier of Windows system files.
- A bundler of proprietary AMD/Intel/NVIDIA binaries. SDKs are fetched by the user on first use.

## Honest capability matrix

| Technology | Requires | External-only mechanism | UFX support |
| --- | --- | --- | --- |
| FSR 1 (spatial) | Post-process hook | Reshade-style plugin (per-game) | Config only |
| FSR 2 | Motion vectors + depth | Game integration | Config / DLL swap where public wrappers exist |
| FSR 3 (upscaling) | Same as FSR 2 | Game integration | Config / DLL swap |
| FSR 3 Frame Generation | Swapchain + motion vectors | Game integration | Config only |
| FSR 4 | RDNA 4 hardware + game update | Game integration | Detected, marked if unavailable |
| XeSS 1.x | Motion vectors + depth | Game integration or DP4a fallback | Config / DLL swap |
| XeSS Frame Generation | Game integration | Game integration | Config only |
| DLSS ↔ FSR3 wrappers | Public wrapper DLL | Per-game DLL swap | Optional, opt-in, with backup |

Rows populated from vendor public docs as of Q1 2026; verify at runtime via `capabilities.json`.

## Modules

- `app/core` — Application services, DI container, config paths.
- `app/gpu` — GPU enumeration via DXGI + WMI, feature-level probe.
- `app/dx11` / `app/dx12` / `app/vulkan` — Feature-level & capability probes only. No hooking of running games.
- `app/upscaling` — `IUpscaler` interface + FSR/XeSS/Null implementations.
- `app/framegen` — `IFrameGenerator` interface + FSR3-FG/XeSS-FG stubs marked "game integration required."
- `app/profiles` — JSON profile load/save with schema validation.
- `app/logging` — Structured logs to `%LOCALAPPDATA%\UniversalFrameFX\logs`.
- `app/ui` — Win32 + WinUI 3 (via XAML Islands fallback to plain Direct2D if WinUI unavailable).
- `installer/` — `install.ps1`, `uninstall.ps1`.
- `website/` — Next.js 15 + Tailwind marketing site.
- `tests/` — GoogleTest suite with mock GPU providers.

## Data flow

```
DXGI/WMI/D3D  →  GpuEnumerator  →  CapabilityMatrix
                                        │
UI  ──────────────────────────────────►┤
                                        ▼
                                 ProfileService  ─►  %APPDATA%\UniversalFrameFX\profiles\*.json
                                        │
                              GameIntegrator (opt-in)
                                        ├─►  Backup(game_dir)
                                        └─►  Apply(config edits, DLL swap)
```

## Build

- Toolchain: MSVC 19.40+ (VS 2022 17.10 or VS 2026).
- Standard: C++20.
- Build system: CMake 3.28+.
- Deps: Windows SDK 10.0.22621+, DirectX Headers (Microsoft, MIT), nlohmann/json (MIT), fmt (MIT), GoogleTest (BSD).

## Non-goals

- Kernel drivers.
- Signed system DLL replacement.
- Anti-cheat evasion.
- Fabricated FPS numbers.
