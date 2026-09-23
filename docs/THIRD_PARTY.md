# Third-party components

Universal FrameFX bundles **no proprietary vendor binaries**. All build-time
dependencies are permissively licensed and fetched during the build.

## Build / runtime dependencies

| Component | Purpose | License | Bundled? |
| --- | --- | --- | --- |
| [nlohmann/json](https://github.com/nlohmann/json) | JSON parsing for profiles | MIT | Fetched at build |
| [GoogleTest](https://github.com/google/googletest) | Unit test framework | BSD-3-Clause | Fetched at build (tests only) |
| Microsoft DirectX runtime (d3d11, d3d12, dxgi) | GPU detection & capability probes | Part of the Windows SDK / OS | System, not redistributed |
| Vulkan loader (`vulkan-1.dll`) | Vulkan capability probe (loaded dynamically) | Provided by the GPU driver | System, not redistributed |

## Website dependencies

| Component | License |
| --- | --- |
| Next.js | MIT |
| React / React DOM | MIT |
| Tailwind CSS | MIT |
| TypeScript | Apache-2.0 |

## Vendor SDKs (NOT bundled)

These are **not** included in this repository. If a feature needs one, the app
explains how to obtain it from the vendor legally.

| SDK | Vendor | How obtained |
| --- | --- | --- |
| AMD FidelityFX SDK (FSR) | AMD | Public GitHub (MIT); user/game supplies the runtime |
| Intel XeSS SDK | Intel | Intel's official redistributable |
| NVIDIA DLSS SDK | NVIDIA | NVIDIA's SDK (proprietary EULA) — never redistributed here |

Any DLL-swap wrapper Universal FrameFX offers must itself be publicly and legally
redistributable; the project links to the source rather than hosting proprietary
files.
