# DirectX 11 vs DirectX 12

Universal FrameFX detects and works with both, but they differ in ways that
affect which upscalers and frame-generation options are available.

## DirectX 11

- Higher-level, driver-managed. Simpler for games, less explicit control.
- Temporal upscalers can be integrated (FSR 2/3 and XeSS have DX11 paths in some
  SDK versions) but support is less common and sometimes lower quality because
  DX11 gives the upscaler less direct control over resources.
- **Frame generation** (FSR 3 FG / XeSS FG) generally targets DX12/Vulkan and is
  not offered on DX11.

## DirectX 12

- Low-level, explicit resource and synchronization control.
- The primary target for modern FSR 3 / XeSS integrations and for frame
  generation, which needs a swapchain proxy that DX12 exposes cleanly.
- Requires the game to manage barriers, descriptor heaps, and command lists —
  which is also what lets the upscaler slot in efficiently.

## What UFX reports

For each detected GPU, UFX shows the **feature level** for both APIs (e.g.
`DirectX 12: Supported (FL 12_2)`), read directly from the driver via
`D3D11CreateDevice` / `D3D12CreateDevice`. Feature level matters: some upscaler
shader workloads want Shader Model 6.4+ (DX12) or specific DP4a support, which
UFX probes through `CheckFeatureSupport`.

Practical guidance:

- Prefer the API the game itself uses. UFX does not change a game's rendering API.
- If a game offers both DX11 and DX12 renderers and you want frame generation,
  choose DX12.
