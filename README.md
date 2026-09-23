# Universal FrameFX

A unified, open-source Windows 11 interface for configuring FidelityFX Super Resolution (FSR, FSR 3, and FSR 4 where supported), Intel XeSS, and frame generation across the games you already own.

> **Honest scope:** Universal FrameFX does **not** magically add FSR or XeSS to games that were not built for them. It gives you one place to configure the upscalers a game already supports, manage per-game profiles, and — where public, redistributable wrapper DLLs exist — drop them into a specific game's folder with a full backup and one-click restore. See [`docs/ARCHITECTURE.md`](docs/ARCHITECTURE.md) for the full capability matrix.

## Features

- GPU detection: NVIDIA / AMD / Intel, VRAM, driver version, DirectX feature level, Vulkan availability.
- Upscaling front-end for FSR 1/2/3, FSR 4 (RDNA 4 detected at runtime), XeSS 1.x, plus "no upscaling".
- Frame-generation front-end that clearly labels **which games actually support it**.
- Per-game JSON profiles with import/export.
- Optional game-library scan (Steam / Epic / GOG / Xbox app) — local only, never transmitted.
- Backup / Restore for any game folder Universal FrameFX writes to.
- Local diagnostics export (JSON / text).
- Fluent-styled UI. No fake FPS numbers, no fake support badges.

## Supported GPUs

Any GPU that exposes DirectX 11 Feature Level 11_0 or higher through DXGI. Whether a given upscaler will run on that GPU is decided by the upscaler's own vendor requirements and reported truthfully in the UI.

## Supported APIs

- Direct3D 11
- Direct3D 12
- Vulkan (capability probe; upscaler bring-up is per-vendor SDK)

## Installation

**MSI installer (standard, per-machine):**

1. Download `UniversalFrameFX-x64.msi` from [Releases](https://github.com/ilikemacos/Chopsticks-SUPER-Resolution/releases).
2. Verify its SHA-256 against the `.msi.sha256` posted with the release.
3. Double-click it and follow the wizard. It installs to `Program Files`, adds a
   Start Menu shortcut, and registers an Add/Remove Programs entry. Because it
   installs for all users, Windows requests elevation — that is the only reason
   admin is needed, and it never touches system files, Defender or SmartScreen.

**PowerShell installer (no admin, per-user):**

```powershell
iex "& { $(iwr -useb https://raw.githubusercontent.com/ilikemacos/Chopsticks-SUPER-Resolution/main/installer/install.ps1) }"
```

**Manual (zip):**

1. Download the latest `UniversalFrameFX-x64.zip` from [Releases](https://github.com/ilikemacos/Chopsticks-SUPER-Resolution/releases).
2. Verify the SHA-256 checksum against the value posted with the release.
3. Extract to `%LOCALAPPDATA%\Programs\UniversalFrameFX`.
4. Run `UniversalFrameFX.exe`.

## Building from source

```powershell
git clone https://github.com/OWNER/UniversalFrameFX.git
cd UniversalFrameFX
cmake -S . -B build -A x64
cmake --build build --config Release
```

Requirements:

- Windows 11 22H2 or newer
- Visual Studio 2022 (17.10+) or Visual Studio 2026, "Desktop development with C++" workload
- Windows SDK 10.0.22621 or newer
- CMake 3.28+

## Game integration

- **Config-only mode** (default, safe): Universal FrameFX writes to the game's own `.ini`/`.cfg`/`.json` after taking a backup.
- **DLL-swap mode** (opt-in, per profile): Copies a publicly redistributable wrapper DLL (for example, an FSR3-to-DLSS wrapper distributed under a permissive license) into the game folder. The original file is backed up to `<game>\_UniversalFrameFX_backup\`. Restore is one click.

Universal FrameFX never modifies files outside a game's own folder and never modifies files owned by Windows.

## Limitations

- No general-purpose upscaler injection. Upscalers need motion vectors, depth, and jitter, which only the game engine can supply.
- Frame generation for games that never shipped with it cannot be added by an external tool.
- FSR 4 requires RDNA 4 hardware and a game update that ships FSR 4; UFX will detect and label this honestly.
- Anti-cheat: Never inject into online/multiplayer titles. The default profile refuses to write into folders whose executables are flagged by EAC / BattlEye / VAC signatures.

## Troubleshooting

- Logs: `%LOCALAPPDATA%\UniversalFrameFX\logs\`.
- Diagnostics export: **Diagnostics → Export report** — creates a redacted JSON.
- If an upscaler shows "Unavailable", the message underneath explains exactly which requirement failed.

## Architecture

See [`docs/ARCHITECTURE.md`](docs/ARCHITECTURE.md).

## Contributing

See [`CONTRIBUTING.md`](CONTRIBUTING.md) and [`CODE_OF_CONDUCT.md`](CODE_OF_CONDUCT.md).

## Security

Report vulnerabilities per [`SECURITY.md`](SECURITY.md).

## License

MIT — see [`LICENSE`](LICENSE). Third-party components retain their own licenses; see [`docs/THIRD_PARTY.md`](docs/THIRD_PARTY.md).
