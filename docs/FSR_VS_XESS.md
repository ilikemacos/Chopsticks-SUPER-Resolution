# FSR vs XeSS

Both are temporal upscalers with the same job — reconstruct a high-resolution
image from a lower-resolution render plus motion data — but they differ in
approach, hardware and licensing.

| | AMD FidelityFX Super Resolution | Intel XeSS |
| --- | --- | --- |
| Vendor | AMD | Intel |
| Approach | FSR 2/3: hand-tuned temporal. FSR 4: ML-based. | ML-based (neural network). |
| Hardware | Cross-vendor (FSR 2/3). FSR 4 needs RDNA 4. | XMX path on Intel Arc; DP4a fallback on others. |
| APIs | DX11 (limited), DX12, Vulkan | DX11 (limited), DX12, Vulkan |
| Frame gen | FSR 3 Frame Generation | XeSS Frame Generation |
| Source | Open (FidelityFX SDK, MIT) | XeSS SDK (redistributable runtime) |

## Image quality

- **FSR 2/3** are analytic and run identically on any vendor. Historically
  slightly more prone to ghosting/shimmer than ML methods in hard cases, but
  very widely supported.
- **XeSS** uses a neural network. On Intel Arc it runs on dedicated XMX matrix
  units and looks excellent; on non-Intel GPUs it falls back to a DP4a integer
  path that is a bit slower and slightly lower quality but still good.
- **FSR 4** is AMD's ML upscaler and requires RDNA 4 hardware; where available it
  is a large quality jump over FSR 2/3.

## Which to choose

- **AMD RDNA 4 GPU + game with FSR 4** → FSR 4 for best quality.
- **Intel Arc GPU** → XeSS (XMX path).
- **Any GPU, broad game support** → FSR 3 is the safest cross-vendor choice.
- **Older GPU / DX11 title** → FSR 2, or FSR 1 as a last-resort spatial option.

Universal FrameFX only offers the options your specific hardware + selected API
can actually run, and explains any that are unavailable.
