// Compatibility data. Every cell reflects the vendor's public, documented
// position as of Q1 2026. "Game" means the technology must be integrated by the
// game engine and cannot be added externally by Universal FrameFX.

export type Support = "yes" | "no" | "game" | "partial";

export interface CompatRow {
  tech: string;
  dx11: Support;
  dx12: Support;
  nvidia: Support;
  amd: Support;
  intel: Support;
  note: string;
}

export const compatRows: CompatRow[] = [
  {
    tech: "CSR 1.0 (our own)",
    dx11: "yes",
    dx12: "yes",
    nvidia: "yes",
    amd: "yes",
    intel: "yes",
    note: "Our own proprietary spatial upscaler, derived from FSR 1 (MIT), compiled into the app. Runs on any GPU and API and is the one upscaler here the app runs itself — no game integration. +1.48 dB PSNR over FSR 1. Costs GPU time; generates no frames.",
  },
  {
    tech: "FSR 1 (spatial)",
    dx11: "yes",
    dx12: "yes",
    nvidia: "yes",
    amd: "yes",
    intel: "yes",
    note: "Spatial-only; runs on any GPU. Needs a post-tonemap color image, so still hooks the render target.",
  },
  {
    tech: "FSR 2",
    dx11: "partial",
    dx12: "yes",
    nvidia: "yes",
    amd: "yes",
    intel: "yes",
    note: "Temporal. Requires motion vectors, depth and jitter from the engine — game integration.",
  },
  {
    tech: "FSR 3 (upscaling)",
    dx11: "partial",
    dx12: "yes",
    nvidia: "yes",
    amd: "yes",
    intel: "yes",
    note: "Cross-vendor upscaler. Game integration required. Config-only or DLL-swap where a public wrapper exists.",
  },
  {
    tech: "FSR 3 Frame Generation",
    dx11: "no",
    dx12: "yes",
    nvidia: "yes",
    amd: "yes",
    intel: "yes",
    note: "Swapchain proxy + motion vectors. Cannot be added to a game that did not ship it.",
  },
  {
    tech: "FSR 4",
    dx11: "no",
    dx12: "yes",
    nvidia: "no",
    amd: "partial",
    intel: "no",
    note: "ML upscaler. Requires AMD RDNA 4 hardware and a game that ships FSR 4.",
  },
  {
    tech: "XeSS 1.x",
    dx11: "partial",
    dx12: "yes",
    nvidia: "yes",
    amd: "yes",
    intel: "yes",
    note: "XMX path on Intel Arc; DP4a fallback elsewhere. Game integration required.",
  },
  {
    tech: "XeSS Frame Generation",
    dx11: "no",
    dx12: "yes",
    nvidia: "partial",
    amd: "partial",
    intel: "yes",
    note: "Game integration required. Best on Intel Arc; other GPUs depend on the game's implementation.",
  },
];

export const supportLabel: Record<Support, string> = {
  yes: "Yes",
  no: "No",
  game: "Game integration",
  partial: "Partial",
};
