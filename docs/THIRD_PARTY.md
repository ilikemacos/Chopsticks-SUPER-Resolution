# Third-party components

Universal FrameFX bundles **no proprietary vendor binaries**. All build-time
dependencies are permissively licensed and fetched during the build.

## CSR (Chopsticks Super Resolution) — our component, proprietary

CSR 1.0 is **proprietary** (see `csr/LICENSE`), licensed separately from the rest
of this repository (which is MIT). It is derived from AMD FidelityFX Super
Resolution 1 (EASU/RCAS, MIT); AMD's copyright and permission notice are retained
verbatim in `csr/NOTICE.md` and ship with CSR in every form, as MIT requires.
CSR bundles no vendor binary — it is a clean-room re-implementation of the FSR 1
algorithm.

## Build / runtime dependencies

| Component | Purpose | License | Bundled? |
| --- | --- | --- | --- |
| [nlohmann/json](https://github.com/nlohmann/json) | JSON parsing for profiles | MIT | Fetched at build |
| [GoogleTest](https://github.com/google/googletest) | Unit test framework | BSD-3-Clause | Fetched at build (tests only) |
| [stb (stb_image / stb_image_write)](https://github.com/nothings/stb) | Image decode/encode for the `ufx-upscale` tool | Public domain (MIT alt.) | Fetched at build |
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
